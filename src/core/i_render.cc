#include "i_render.h"

namespace n_render
{
  void createIRender(IRender &iRender, Context &context, Window &window)
  {
    iRender.m_context = &context;
    iRender.m_window  = &window;
  }

  void draw_IRender(IRender &iRender)
  {
    return;
  }

  void destoryIRender(IRender &iRender)
  {
    return;
  }
}
