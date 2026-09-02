#pragma once

struct Context;
struct Window;

struct IRender
{
  Context *m_context;
  Window  *m_window;
};

namespace n_render
{
  void createIRender(IRender &iRender, Context &context, Window &window);
  void draw_IRender(IRender &iRender);
  void destoryIRender(IRender &iRender);
}
