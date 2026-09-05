#ifndef PA_SCREEN_H
#define PA_SCREEN_H

namespace PA::UI {

class Screen {
public:
  Screen() = default;
  virtual ~Screen() = default;
  
  // The intent is to wrap up the logic for the particular screen logic
  // here and have it render a proper FTXUI element. 
  virtual void Render() = 0;

  // The intent is any init needed before actually rendering
  virtual void Init() = 0;

  // The intent is any clean up needed before destruction
  virtual void Cleanup() = 0;

protected:
  Screen m_NextScreen;
};

}

#endif
