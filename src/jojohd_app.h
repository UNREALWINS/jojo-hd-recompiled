// jojohd - ReXGlue Recompiled Project
// Public host application wrapper. Original game code/assets are not included.

#pragma once

#include <rex/rex_app.h>

class JojohdApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<JojohdApp>(
        new JojohdApp(ctx, "jojohd", PPCImageConfig));
  }
};
