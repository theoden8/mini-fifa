#pragma once

template <typename T, typename X> struct Sprite;

template <typename T, typename X>
Sprite<T, X> *&getInstance() {
  static Sprite<T, X> *instance = nullptr;
  return instance;
}

template <typename T, typename X>
struct Sprite {
  bool initialized = false;
  T object;

  template <typename... Args>
  Sprite(Args&&... args):
    object(args...)
  {}

  template <typename... Args>
  static Sprite<T, X> *create(Args&&... args) {
    if(getInstance<T, X>() == nullptr) {
      getInstance<T, X>() = new Sprite<T, X>(args...);
    }
    return getInstance<T, X>();
  }

  static void init() {
    if(!getInstance<T, X>()->initialized) {
      getInstance<T, X>()->object.init();
      getInstance<T, X>()->initialized = true;
    }
  }

  template <typename... Args>
  static void display(Args&&... args) {
    getInstance<T, X>()->object.display(args...);
  }

  static void clear() {
    if(getInstance<T, X>() != nullptr) {
      getInstance<T, X>()->object.clear();
      delete getInstance<T, X>();
      getInstance<T, X>() = nullptr;
      getInstance<T, X>()->initialized = false;
    }
  }
};

