#ifndef WSON_BASE_BUFFER_H_
#define WSON_BASE_BUFFER_H_

#include "types.h"

class BaseBuffer {

  public:

    inline void push(uint16_t c) {
      buffer_.push_back(c);
    }

    template<typename S>
    void append(const S& source, int start=0, int length=-1) {
      if (length < 0) {
        length = source.size() - start;
      }
      typename S::const_iterator sourceBegin = source.begin() + start;
      typename S::const_iterator sourceEnd = sourceBegin + length;
      buffer_.reserve(buffer_.size() + length);
      buffer_.insert(buffer_.end(), sourceBegin, sourceEnd);
    }

    inline void appendHandle(v8::Local<v8::String> source, int start=0, int length=-1) {
      v8::Isolate* isolate = v8::Isolate::GetCurrent();
      size_t oldSize = buffer_.size();
      if (length < 0) {
        length = source->Length() - start;
      }
      buffer_.resize(oldSize + length);
      source->Write(isolate, buffer_.data() + oldSize, start, length, v8::String::NO_NULL_TERMINATION);
    }

    inline v8::Local<v8::String> getHandle() const {
      // return Nan::New<v8::String>(buffer_.data(), buffer_.size());
      return Nan::New<v8::String>(buffer_.data(), buffer_.size()).ToLocalChecked();
    }

    inline const usc2vector& getBuffer() const {
      return buffer_;
    }

    size_t size() const {
      return buffer_.size();
    }

    void reserve(size_t x) {
      buffer_.reserve(x);
    }

    static inline uint16_t getEscapeChar(uint16_t c) {
      switch (c) {
        case '{':
          return 'o';
        case '}':
          return 'c';
        case '[':
          return 'a';
        case ']':
          return 'e';
        case ':':
          return 'i';
        case '#':
          return 'l';
        case '|':
          return 'p';
        case '`':
          return 'q';
      }
      return 0;
    }

    static inline uint16_t getUnescapeChar(uint16_t c) {
      switch (c) {
        case 'o':
          return '{';
        case 'c':
          return '}';
        case 'a':
          return '[';
        case 'e':
          return ']';
        case 'i':
          return ':';
        case 'l':
          return '#';
        case 'p':
          return '|';
        case 'q':
          return '`';
      }
      return 0;
    }

    inline void clear() {
      buffer_.resize(0);
    }

  protected:
    usc2vector buffer_;
};

#endif // WSON_BASE_BUFFER_H_
