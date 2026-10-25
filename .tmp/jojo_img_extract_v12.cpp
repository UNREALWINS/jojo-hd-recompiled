#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "lzx.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

static uint16_t be16(const uint8_t* p) { return (uint16_t(p[0]) << 8) | p[1]; }
static uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}

static std::vector<uint8_t> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    auto n = f.tellg();
    f.seekg(0, std::ios::beg);
    if (n <= 0) return {};
    std::vector<uint8_t> b(static_cast<size_t>(n));
    f.read(reinterpret_cast<char*>(b.data()), n);
    return b;
}

static bool write_file(const fs::path& p, const std::vector<uint8_t>& b) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
    return !!f;
}

static std::string sanitize_component(std::string s) {
    for (char& c : s) {
        if (c == '\\') c = '/';
        if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') c = '_';
    }
    while (!s.empty() && (s.front() == '/' || s.front() == '.')) s.erase(s.begin());
    return s;
}

static int log2_bpp(int texelPitch) {
    return (texelPitch >> 2) + ((texelPitch >> 1) >> (texelPitch >> 2));
}

static int tiled_x(int offset, int width, int texelPitch) {
    const int alignedWidth = (width + 31) & ~31;
    const int logBpp = log2_bpp(texelPitch);
    const int offsetB = offset << logBpp;
    const int offsetT = ((offsetB & ~4095) >> 3) + ((offsetB & 1792) >> 2) + (offsetB & 63);
    const int offsetM = offsetT >> (7 + logBpp);
    const int macroX = (offsetM % (alignedWidth >> 5)) << 2;
    const int tile = (((offsetT >> (5 + logBpp)) & 2) + (offsetB >> 6)) & 3;
    const int macro = (macroX + tile) << 3;
    const int micro = ((((offsetT >> 1) & ~15) + (offsetT & 15)) & ((texelPitch << 3) - 1)) >> logBpp;
    return macro + micro;
}

static int tiled_y(int offset, int width, int texelPitch) {
    const int alignedWidth = (width + 31) & ~31;
    const int logBpp = log2_bpp(texelPitch);
    const int offsetB = offset << logBpp;
    const int offsetT = ((offsetB & ~4095) >> 3) + ((offsetB & 1792) >> 2) + (offsetB & 63);
    const int offsetM = offsetT >> (7 + logBpp);
    const int macroY = (offsetM / (alignedWidth >> 5)) << 2;
    const int tile = ((offsetT >> (6 + logBpp)) & 1) + ((offsetB & 2048) >> 10);
    const int macro = (macroY + tile) << 3;
    const int micro = ((((offsetT & (((texelPitch << 6) - 1) & ~31)) + ((offsetT & 15) << 1)) >> (3 + logBpp)) & ~1);
    return macro + micro + ((offsetT & 16) >> 4);
}

static size_t aligned_surface_size(int width, int height, int pitch) {
    const size_t aw = size_t((width + 31) & ~31);
    const size_t ah = size_t((height + 31) & ~31);
    return aw * ah * size_t(pitch);
}

static std::vector<uint8_t> untile_blocks(const uint8_t* data, size_t dataSize, int blockWidth, int blockHeight, int pitch) {
    const size_t logical = size_t(blockWidth) * blockHeight * pitch;
    std::vector<uint8_t> out(logical, 0);
    const size_t sourceCount = dataSize / size_t(pitch);
    for (size_t blockOffset = 0; blockOffset < sourceCount; ++blockOffset) {
        const int x = tiled_x(int(blockOffset), blockWidth, pitch);
        const int y = tiled_y(int(blockOffset), blockWidth, pitch);
        if (x < 0 || y < 0 || x >= blockWidth || y >= blockHeight) continue;
        const size_t src = blockOffset * size_t(pitch);
        const size_t dst = (size_t(y) * blockWidth + size_t(x)) * size_t(pitch);
        if (src + pitch <= dataSize && dst + pitch <= out.size()) {
            std::memcpy(out.data() + dst, data + src, pitch);
        }
    }
    return out;
}

static void swap16(std::vector<uint8_t>& b) {
    for (size_t i = 0; i + 1 < b.size(); i += 2) std::swap(b[i], b[i + 1]);
}

struct RGBA { uint8_t r, g, b, a; };

static RGBA rgb565(uint16_t v) {
    return {
        uint8_t(((v >> 11) & 31) * 255 / 31),
        uint8_t(((v >> 5) & 63) * 255 / 63),
        uint8_t((v & 31) * 255 / 31),
        255
    };
}

static RGBA mix(const RGBA& a, const RGBA& b, int wa, int wb, int div) {
    return { uint8_t((wa*a.r+wb*b.r)/div), uint8_t((wa*a.g+wb*b.g)/div), uint8_t((wa*a.b+wb*b.b)/div), 255 };
}

static void put_px(std::vector<uint8_t>& img, int w, int h, int x, int y, const RGBA& c) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    size_t o = (size_t(y) * w + x) * 4;
    img[o]=c.r; img[o+1]=c.g; img[o+2]=c.b; img[o+3]=c.a;
}

static std::array<RGBA,4> dxt_colors(const uint8_t* p, bool forceFour) {
    uint16_t c0 = uint16_t(p[0]) | (uint16_t(p[1]) << 8);
    uint16_t c1 = uint16_t(p[2]) | (uint16_t(p[3]) << 8);
    RGBA a = rgb565(c0), b = rgb565(c1);
    std::array<RGBA,4> pal{};
    pal[0]=a; pal[1]=b;
    if (c0 > c1 || forceFour) {
        pal[2]=mix(a,b,2,1,3); pal[3]=mix(a,b,1,2,3);
    } else {
        pal[2]=mix(a,b,1,1,2); pal[3]={0,0,0,0};
    }
    return pal;
}

static bool decode_dxt1(const std::vector<uint8_t>& blocks, int w, int h, std::vector<uint8_t>& img) {
    int bw=(w+3)/4, bh=(h+3)/4;
    if (blocks.size() < size_t(bw)*bh*8) return false;
    img.assign(size_t(w)*h*4,0);
    for (int by=0;by<bh;++by) for(int bx=0;bx<bw;++bx) {
        const uint8_t* p=&blocks[(size_t(by)*bw+bx)*8];
        auto pal=dxt_colors(p,false);
        uint32_t bits=uint32_t(p[4])|(uint32_t(p[5])<<8)|(uint32_t(p[6])<<16)|(uint32_t(p[7])<<24);
        for(int yy=0;yy<4;++yy) for(int xx=0;xx<4;++xx) {
            int idx=(bits>>(2*(yy*4+xx)))&3; put_px(img,w,h,bx*4+xx,by*4+yy,pal[idx]);
        }
    }
    return true;
}

static bool decode_dxt3(const std::vector<uint8_t>& blocks, int w, int h, std::vector<uint8_t>& img) {
    int bw=(w+3)/4, bh=(h+3)/4;
    if (blocks.size() < size_t(bw)*bh*16) return false;
    img.assign(size_t(w)*h*4,0);
    for (int by=0;by<bh;++by) for(int bx=0;bx<bw;++bx) {
        const uint8_t* p=&blocks[(size_t(by)*bw+bx)*16];
        auto pal=dxt_colors(p+8,true);
        uint32_t bits=uint32_t(p[12])|(uint32_t(p[13])<<8)|(uint32_t(p[14])<<16)|(uint32_t(p[15])<<24);
        uint64_t alpha=0; for(int i=0;i<8;++i) alpha|=uint64_t(p[i])<<(8*i);
        for(int yy=0;yy<4;++yy) for(int xx=0;xx<4;++xx) {
            int q=yy*4+xx, idx=(bits>>(2*q))&3; RGBA c=pal[idx]; c.a=uint8_t(((alpha>>(4*q))&15)*17);
            put_px(img,w,h,bx*4+xx,by*4+yy,c);
        }
    }
    return true;
}

static bool decode_dxt5(const std::vector<uint8_t>& blocks, int w, int h, std::vector<uint8_t>& img) {
    int bw=(w+3)/4, bh=(h+3)/4;
    if (blocks.size() < size_t(bw)*bh*16) return false;
    img.assign(size_t(w)*h*4,0);
    for (int by=0;by<bh;++by) for(int bx=0;bx<bw;++bx) {
        const uint8_t* p=&blocks[(size_t(by)*bw+bx)*16];
        uint8_t a0=p[0], a1=p[1], ap[8]{}; ap[0]=a0; ap[1]=a1;
        if(a0>a1){for(int i=1;i<=6;++i) ap[i+1]=uint8_t(((7-i)*a0+i*a1)/7);} else {for(int i=1;i<=4;++i) ap[i+1]=uint8_t(((5-i)*a0+i*a1)/5); ap[6]=0; ap[7]=255;}
        uint64_t abits=0; for(int i=0;i<6;++i) abits|=uint64_t(p[2+i])<<(8*i);
        auto pal=dxt_colors(p+8,true);
        uint32_t bits=uint32_t(p[12])|(uint32_t(p[13])<<8)|(uint32_t(p[14])<<16)|(uint32_t(p[15])<<24);
        for(int yy=0;yy<4;++yy) for(int xx=0;xx<4;++xx){int q=yy*4+xx,idx=(bits>>(2*q))&3,ai=(abits>>(3*q))&7;RGBA c=pal[idx];c.a=ap[ai];put_px(img,w,h,bx*4+xx,by*4+yy,c);}    }
    return true;
}

static bool tex_to_png(const std::vector<uint8_t>& tex, const fs::path& outPng, std::string& why) {
    if (tex.size() < 20 || !(tex[0]==0 && tex[1]=='X' && tex[2]=='E' && tex[3]=='T')) { why="not TEX"; return false; }
    uint32_t packed=be32(tex.data()+8);
    int mipCount=int(packed & 0x3F);
    int width=int((packed & 0x0007FFC0u)>>6);
    int height=int((packed & 0xFFF80000u)>>19);
    uint8_t fmt=tex[14];
    if(width<=0||height<=0||mipCount<=0){why="invalid dimensions";return false;}
    uint32_t dataOff=be32(tex.data()+16);
    if(dataOff<16 || dataOff>=tex.size()) { dataOff=16+4u*uint32_t(mipCount); }
    if(dataOff>=tex.size()){why="bad pixel offset";return false;}

    int pitch=0; enum Kind{K_DXT1,K_DXT3,K_DXT5,K_BGRA8,K_RGBA8_LINEAR,K_A8} kind;
    int elemW=0, elemH=0;
    if(fmt==0x12){pitch=1;kind=K_A8;elemW=width;elemH=height;}
    else if(fmt==0x13||fmt==0x14){pitch=8;kind=K_DXT1;elemW=(width+3)/4;elemH=(height+3)/4;}
    else if(fmt==0x15){pitch=16;kind=K_DXT3;elemW=(width+3)/4;elemH=(height+3)/4;}
    else if(fmt==0x17||fmt==0x18||fmt==0x2A||fmt==0x2B){pitch=16;kind=K_DXT5;elemW=(width+3)/4;elemH=(height+3)/4;}
    else if(fmt==0x27){pitch=4;kind=K_BGRA8;elemW=width;elemH=height;}
    else { std::ostringstream os; os<<"unsupported TEX format 0x"<<std::hex<<int(fmt); why=os.str(); return false; }

    size_t avail=tex.size()-dataOff;
    size_t surface=aligned_surface_size(elemW,elemH,pitch);
    size_t linearStride=0;
    if(kind==K_BGRA8){
        linearStride=size_t((width+31)&~31)*size_t(height)*4u;
        if(avail<surface && avail>=linearStride) kind=K_RGBA8_LINEAR;
    }
    if(kind!=K_RGBA8_LINEAR && avail<surface){why="pixel data too small";return false;}
    std::vector<uint8_t> linear;
    if(kind==K_RGBA8_LINEAR) linear.assign(tex.begin()+dataOff, tex.begin()+dataOff+linearStride);
    else linear=untile_blocks(tex.data()+dataOff,surface,elemW,elemH,pitch);
    std::vector<uint8_t> rgba;
    bool ok=false;
    if(kind==K_BGRA8){
        rgba.resize(size_t(width)*height*4);
        for(size_t i=0;i<size_t(width)*height;++i){
            const uint8_t* q=&linear[i*4];
            uint8_t* d=&rgba[i*4];
            d[0]=q[3]; d[1]=q[0]; d[2]=q[1]; d[3]=q[2];
        }
        ok=true;
    } else if(kind==K_RGBA8_LINEAR){
        const size_t stride=size_t((width+31)&~31)*4u;
        rgba.resize(size_t(width)*height*4);
        for(int y=0;y<height;++y){
            std::memcpy(rgba.data()+size_t(y)*width*4, linear.data()+size_t(y)*stride, size_t(width)*4);
        }
        ok=true;
    } else if(kind==K_A8){
        rgba.resize(size_t(width)*height*4);
        uint8_t maxv=0;
        for(size_t i=0;i<size_t(width)*height;++i) if(linear[i]>maxv) maxv=linear[i];
        const bool fourBitRange = maxv <= 15;
        for(size_t i=0;i<size_t(width)*height;++i){
            uint8_t v = fourBitRange ? uint8_t(linear[i]*17) : linear[i];
            uint8_t* d=&rgba[i*4];
            d[0]=v; d[1]=v; d[2]=v; d[3]=255;
        }
        ok=true;
    } else {
        swap16(linear);
        ok = kind==K_DXT1?decode_dxt1(linear,width,height,rgba):kind==K_DXT3?decode_dxt3(linear,width,height,rgba):decode_dxt5(linear,width,height,rgba);
    }
    if(!ok){why="texture decode failed";return false;}
    std::error_code ec; fs::create_directories(outPng.parent_path(),ec);
    if(!stbi_write_png(outPng.string().c_str(),width,height,4,rgba.data(),width*4)){why="PNG write failed";return false;}
    std::ostringstream os; os<<width<<"x"<<height<<" fmt=0x"<<std::hex<<int(fmt); if(kind==K_RGBA8_LINEAR) os<<" linear"; why=os.str(); return true;
}

static bool xmem_decompress(const uint8_t* src, size_t srcSize, size_t expected, std::vector<uint8_t>& out, std::string& why) {
    out.assign(expected,0);
    lzx_state* st=lzx_init(15);
    if(!st){why="lzx_init failed";return false;}
    size_t ip=0,op=0; bool ok=true;
    while(ip<srcSize && op<expected){
        uint32_t dst=0, comp=0; size_t header=0,suffix=0;
        if(src[ip]==0xFF){if(ip+5>srcSize){ok=false;break;}dst=be16(src+ip+1);comp=be16(src+ip+3);header=5;suffix=5;}
        else {if(ip+2>srcSize){ok=false;break;}dst=0x8000;comp=be16(src+ip);header=2;}
        if(!dst||!comp||ip+header+comp+suffix>srcSize||op+dst>expected){ok=false;break;}
        int r=lzx_decompress(st,src+ip+header,out.data()+op,int(comp),int(dst));
        if(r!=0){std::ostringstream os;os<<"LZX error "<<r;why=os.str();ok=false;break;}
        ip+=header+comp+suffix; op+=dst;
    }
    lzx_teardown(st);
    if(!ok||op!=expected){if(why.empty())why="XMem size mismatch";return false;}
    return true;
}

struct Stats { uint64_t arcs=0, entries=0, tex=0, png=0, fail=0, directTex=0; };

static void process_tex_blob(const std::vector<uint8_t>& blob, const fs::path& pngPath, const fs::path& fallbackTex, Stats& s, std::ostream& report) {
    s.tex++;
    std::string why;
    if(tex_to_png(blob,pngPath,why)){s.png++;report<<"[PNG] "<<pngPath.string()<<" ("<<why<<")\n";}
    else {s.fail++;write_file(fallbackTex,blob);report<<"[TEX NAO CONVERTIDO] "<<fallbackTex.string()<<" - "<<why<<"\n";}
}

static void process_arc(const fs::path& arc, const fs::path& root, const fs::path& outRoot, Stats& s, std::ostream& report) {
    auto b=read_file(arc); if(b.size()<8||std::memcmp(b.data(),"\0CRA",4)!=0) return;
    uint16_t ver=be16(b.data()+4), count=be16(b.data()+6); if(ver!=17){report<<"[ARC IGNORADO v"<<ver<<"] "<<arc.string()<<"\n";return;}
    if(b.size()<8+size_t(count)*80){report<<"[ARC INVALIDO] "<<arc.string()<<"\n";s.fail++;return;}
    s.arcs++;
    fs::path arcRel=fs::relative(arc,root); arcRel.replace_extension();
    for(uint16_t i=0;i<count;++i){
        size_t e=8+size_t(i)*80;
        std::string name(reinterpret_cast<const char*>(b.data()+e),strnlen(reinterpret_cast<const char*>(b.data()+e),64));
        uint32_t type=be32(b.data()+e+64), csize=be32(b.data()+e+68), dflag=be32(b.data()+e+72), off=be32(b.data()+e+76);
        size_t dsize=size_t(dflag>>3); s.entries++;
        if(off> b.size() || csize>b.size()-off){report<<"[ENTRADA INVALIDA] "<<arc.string()<<" #"<<i<<"\n";s.fail++;continue;}
        std::vector<uint8_t> dec; std::string why;
        if(!xmem_decompress(b.data()+off,csize,dsize,dec,why)){report<<"[LZX FALHOU] "<<arc.string()<<" #"<<i<<" "<<name<<" - "<<why<<"\n";s.fail++;continue;}
        bool isTex = type==0x241F5DEB || (dec.size()>=4 && dec[0]==0 && dec[1]=='X' && dec[2]=='E' && dec[3]=='T');
        if(isTex){
            fs::path rel=fs::path(sanitize_component(name));
            fs::path base=outRoot / arcRel / rel;
            fs::path png=base; png += ".png";
            fs::path raw=outRoot / "_TEX_NAO_CONVERTIDOS" / arcRel / rel; raw += ".tex";
            process_tex_blob(dec,png,raw,s,report);
        }
    }
}

int main(int argc,char** argv){
    fs::path root = argc>=2 ? fs::path(argv[1]) : fs::current_path();
    if(!fs::exists(root)){std::cerr<<"Pasta nao encontrada: "<<root<<"\n";return 2;}
    fs::path outRoot = argc>=3 ? fs::path(argv[2]) : (root / "JOJO_IMAGENS_EXTRAIDAS");
    std::error_code ec; fs::create_directories(outRoot,ec);
    std::ofstream report(outRoot/"RELATORIO.txt",std::ios::binary);
    Stats s;
    std::cout<<"JoJo HD - Extrator de Imagens Xbox 360\n";
    std::cout<<"Origem: "<<root<<"\nSaida:  "<<outRoot<<"\n\n";
    for(auto it=fs::recursive_directory_iterator(root,fs::directory_options::skip_permission_denied,ec);it!=fs::recursive_directory_iterator();it.increment(ec)){
        if(ec){ec.clear();continue;} if(!it->is_regular_file())continue;
        fs::path p=it->path();
        if(p.string().find(outRoot.string())==0) continue;
        std::string ext=p.extension().string(); std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(ext==".arc") process_arc(p,root,outRoot,s,report);
        else if(ext==".tex"){
            auto b=read_file(p); s.directTex++;
            fs::path rel=fs::relative(p,root); fs::path png=outRoot/rel; png.replace_extension(".png");
            fs::path raw=outRoot/"_TEX_NAO_CONVERTIDOS"/rel;
            process_tex_blob(b,png,raw,s,report);
        }
    }
    report<<"\nRESUMO\nARC: "<<s.arcs<<"\nEntradas ARC: "<<s.entries<<"\nTEX: "<<s.tex<<"\nPNG: "<<s.png<<"\nFalhas/nao suportados: "<<s.fail<<"\n";
    std::cout<<"Concluido.\nARC processados: "<<s.arcs<<"\nEntradas ARC: "<<s.entries<<"\nTexturas encontradas: "<<s.tex<<"\nPNGs criados: "<<s.png<<"\nNao convertidas/falhas: "<<s.fail<<"\n\nVeja: "<<(outRoot/"RELATORIO.txt")<<"\n";
    return s.png?0:3;
}
