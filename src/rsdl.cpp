#include "rsdl.hpp"
#include <iostream>
#include <sstream>

using namespace std;

void delay(int millisecond) { SDL_Delay(millisecond); }

Event::Event() {}

Event::Event(SDL_Event _sdlEvent) { sdlEvent = _sdlEvent; }

Event::EventType Event::get_type() const {
  SDL_Event e = sdlEvent;
  try {
    if (e.type == SDL_QUIT)
      return QUIT;
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
      return LCLICK;
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
      return RCLICK;
    if (e.type == SDL_KEYDOWN)
      return KEY_PRESS;
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
      return LRELEASE;
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
      return RRELEASE;
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
      return RRELEASE;
    if (e.type == SDL_MOUSEMOTION)
      return MMOTION;
  } catch (...) {
    return NA;
  }
  return NA;
}

Point Event::get_mouse_position() const {
  switch (get_type()) {
  case MMOTION:
    return Point(sdlEvent.motion.x, sdlEvent.motion.y);
  case LCLICK:
  case RCLICK:
  case LRELEASE:
  case RRELEASE:
    return Point(sdlEvent.button.x, sdlEvent.button.y);
  default:
    throw "Invalid Event Type";
  }
}

Point get_current_mouse_position() {
  Point ret(0, 0);
  SDL_GetMouseState(&ret.x, &ret.y);
  return ret;
}

Point Event::get_relative_mouse_position() const {
  switch (get_type()) {
  case MMOTION:
    return Point(sdlEvent.motion.x, sdlEvent.motion.y);
  default:
    return Point(0, 0);
  }
}

char Event::get_pressed_key() const {
  if (get_type() != KEY_PRESS)
    return -1;
  return sdlEvent.key.keysym.sym;
}

void Window::init() {
  if (SDL_Init(0) < 0)
    throw "SDL Init Fail";
  int flags = (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  if (SDL_WasInit(flags) != 0)
    throw string("SDL_WasInit Failed ") + SDL_GetError();
  if (SDL_InitSubSystem(flags) < 0)
    throw string("SDL_InitSubSystem Failed ") + SDL_GetError();
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    throw "IMG_Init Fail";
  if (TTF_Init() == -1)
    throw "TTF_Init Fail";
}

Window::Window(Point _size, std::string title) : size(_size) {
  init();
  SDL_CreateWindowAndRenderer(size.x, size.y, 0, &win, &renderer);
  if (win == NULL || renderer == NULL)
    throw string("Window could not be created! SDL_Error: ") + SDL_GetError();
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_SetWindowTitle(win, title.c_str());
  update_screen();
}

Window::~Window() {
  SDL_DestroyWindow(win);
  if (TTF_WasInit())
    TTF_Quit();
  SDL_Quit();
}

Window &Window::operator=(const Window &window) {
  size = window.size;
  return *this;
}

void Window::show_text(string input, Point src, RGB color, string font_addr,
                       int size) {
  SDL_Color textColor = {
      color.red,
      color.green,
      color.blue,
  };
  stringstream ss;
  ss << size;
  TTF_Font *font = fonts_cache[font_addr + ":" + ss.str()];
  if (font == NULL) {
    font = TTF_OpenFont(font_addr.c_str(), size);
    fonts_cache[font_addr + ":" + ss.str()] = font;
    if (font == NULL)
      throw "Font Not Found: " + font_addr;
  }
  SDL_Surface *textSurface =
      TTF_RenderText_Solid(font, input.c_str(), textColor);
  SDL_Texture *text = SDL_CreateTextureFromSurface(renderer, textSurface);
  SDL_Rect renderQuad = {src.x, src.y, textSurface->w, textSurface->h};
  SDL_RenderCopy(renderer, text, NULL, &renderQuad);
  SDL_DestroyTexture(text);
  SDL_FreeSurface(textSurface);
}

void Window::set_color(RGB color) {
  SDL_SetRenderDrawColor(renderer, color.red, color.green, color.blue, 255);
}

void Window::clear() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
}

void Window::draw_img(string filename, Point src, Point size, double angle,
                      bool flip_horizontal, bool flip_vertical) {
  SDL_Texture *res = texture_cache[filename];
  if (res == NULL) {
    res = IMG_LoadTexture(renderer, filename.c_str());
    texture_cache[filename] = res;
  }
  SDL_RendererFlip flip = (SDL_RendererFlip)(
      (flip_horizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE) |
      (flip_vertical ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE));
  SDL_Rect dst = {src.x, src.y, size.x ? size.x : this->size.x,
                  size.y ? size.y : this->size.y};
  SDL_RenderCopyEx(renderer, res, NULL, &dst, angle, NULL, flip);
}

void Window::update_screen() { SDL_RenderPresent(renderer); }

void Window::fill_rect(Point src, Point size, RGB color) {
  set_color(color);
  SDL_Rect r = {src.x, src.y, size.x, size.y};
  SDL_RenderFillRect(renderer, &r);
}

void Window::draw_line(Point src, Point dst, RGB color) {
  set_color(color);
  SDL_RenderDrawLine(renderer, src.x, src.y, dst.x, dst.y);
}

void Window::draw_point(Point p, RGB color) {
  set_color(color);
  SDL_RenderDrawPoint(renderer, p.x, p.y);
}

void Window::draw_rect(Point src, Point size, RGB color,
                       unsigned int line_width) {
  for (size_t i = 0; i < line_width; i++) {
    draw_line(src + Point(i, i), src + Point(size.x - i, i), color);
    draw_line(src + Point(i, i), src + Point(i, size.y - i), color);
    draw_line(src + Point(i, size.y - i), src + size - Point(i, i), color);
    draw_line(src + Point(size.x - i, i), src + size - Point(i, i), color);
  }
}

void Window::fill_circle(Point center, int r, RGB color) {
  float tx, ty;
  float xr;
  set_color(color);
  for (ty = (float)-SDL_fabs(r); ty <= (float)SDL_fabs((int)r); ty++) {
    xr = (float)SDL_sqrt(r * r - ty * ty);
    if (r > 0) { /* r > 0 ==> filled circle */
      for (tx = -xr + .5f; tx <= xr - .5; tx++) {
        draw_point(center + Point(tx, ty), color);
      }
    } else {
      draw_point(center + Point(-xr + .5f, ty), color);
      draw_point(center + Point(+xr - .5f, ty), color);
    }
  }
}

Event Window::poll_for_event() {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    Event e(event);
    if (e.get_type() != 0)
      return e;
  }
  return event;
}

RGB::RGB(int r, int g, int b) {
  if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
    throw "Invalid RGB Color";
  red = r;
  green = g;
  blue = b;
}

Point::Point(int _x, int _y) : x(_x), y(_y) {}

void Window::dump_err() { cerr << SDL_GetError() << endl; }

Point Point::operator+(const Point p) const { return Point(x + p.x, y + p.y); }

Point Point::operator-(const Point p) const { return (*this) + (-1) * p; }

Point Point::operator*(const int c) const { return Point(x * c, y * c); }

Point Point::operator/(const int c) const { return Point(x / c, y / c); }

Point operator*(const int c, const Point p) { return p * c; }

Point &Point::operator+=(const Point p) {
  (*this) = (*this) + p;
  return (*this);
}

Point &Point::operator-=(const Point p) {
  (*this) = (*this) - p;
  return (*this);
}

Point &Point::operator*=(const int c) {
  (*this) = (*this) * c;
  return (*this);
}

Point &Point::operator/=(const int c) {
  (*this) = (*this) / c;
  return (*this);
}

Point::operator SDL_Point() {
  SDL_Point ret;
  ret.x = x;
  ret.y = y;
  return ret;
}

ostream &operator<<(ostream &stream, const Point p) {
  stream << '(' << p.x << ", " << p.y << ')';
  return stream;
}
