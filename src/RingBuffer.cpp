#include <Arduino.h>

template<typename T> class RingBuffer
{
  public:
    RingBuffer(int size, T threshold) : RingBuffer(size, threshold, false){}

    RingBuffer(int size, T threshold, bool writeDebug)
    {
      _values = new T[size];
      _threshold = threshold;
      _size = size; 
      _writeDebug = writeDebug;
      _initialized = false;
      Fill(0);
    }

    ~RingBuffer()
    {
      delete[] _values;
    }

    void Put(T value)
    {
      if(!_initialized)
      {
        Fill(value);
        _initialized = true;
      }
      
      if (abs(value - Newest()) < _threshold)
        return;
        
      _values[_cursor++] = value;
      if (_cursor == _size) _cursor = 0;
      if (_writeDebug)
      {
         Serial.print("Put: ");
         Serial.print(value);
         Serial.print(" Content: ");
         for (int i = 0; i < _size; i++)
         {
          Serial.print(Get(i));
          Serial.print(", ");
         }
         Serial.print("\n");
      }
    }

    double Average()
    {
      T total = 0;
      for (int i = 0; i < _size; i++)
        total += _values[i];

      return (double)total / (double)_size;
    }

    T Get(int index)
    {
      int idx = index + _cursor;
      if (idx >= _size) idx -= _size;
      return _values[idx];
    }

    T Size()
    {
      return _size;
    }

    T Oldest()
    {
      return _values[_cursor];
    }

    T Newest()
    {
      int lastIdx = _cursor - 1;
      if (lastIdx < 0) lastIdx = _size - 1;
      return _values[lastIdx];
    }

    void Fill(T value)
    {
      for (int i = 0; i < _size; i++) _values[i] = value;
    }
    
  private:
    T* _values;
    int _cursor;
    int _size;
    T _threshold;
    bool _writeDebug;
    bool _initialized;
};
