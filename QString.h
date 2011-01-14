#pragma once

#ifndef _QSTRING_H_
#define _QSTRING_H_

#include <stdlib.h>
#include <malloc.h>

#if !defined(QSTRING_HEX)
#define QSTRING_HEX 0
#endif

#if QSTRING_HEX
class QHex
{
   static inline int to_hex(UCHAR data) { return HexTable[data]; }
   static inline int to_hex(const char *data) { return (HexTable[data[0]] << 4) + HexTable[data[1]]; }
   static inline int to_hex_rev(const char *data) { return HexTable[data[0]] + (HexTable[data[1]] << 4); }
   static inline bool isxdigit(UCHAR c) { return HexTable[c] | (c == '0'); }
                    
private:
   static UCHAR HexTable[256];
};

#if defined(QSTRING_MAIN) && QSTRING_MAIN
const ULONG unsigned int QHex::HexTable[256] = {
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 1, 2, 3,  4, 5, 6, 7,  8, 9, 0, 0,  0, 0, 0, 0,
   10,11,12,13, 14,15, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
   10,11,12,13, 14,15 };
#endif // QSTRING_MAIN
#endif // QSTRING_HEX

typedef size_t q_size_t;

// This is a quick string class.
// Its goal is to relieve the caller of memory allocation duties
// while providing full access to the string data.
// Adam Bailey    11/23/04
// Added some new features and copied to snippets 10/10/05
class QString
{
public:
   char *data;     // pointer to string data
   q_size_t length;     // length of string
   q_size_t bufferSize; // size of buffer

   QString()
   {
      data = NULL;
      length = bufferSize = 0;
   }

   QString(const char *src, int srcLength = -1)
   {
      data = NULL;
      length = bufferSize = 0;
      Append(src, srcLength);
   }

   QString(const int newLength)
   {
      data = NULL;
      length = bufferSize = 0;
      Grow(newLength);
   }

   QString(QString &src)
   {
       data = NULL;
       length = bufferSize = 0;
       Append(src.data, src.length);
   }

   QString(HWND hWnd)
   {
       data = NULL;
       length = bufferSize = 0;
       int newLen = GetWindowTextLength(hWnd);
       Grow(newLen);
       GetWindowTextA(hWnd, data, newLen + 1);
   }

   ~QString()
   {
      free(data);
   }

   inline operator const char*() const { return c_str(); }
   inline operator       char*() const { return data; }
   inline char operator [](int index) const { return At(index); }

   const char *c_str() const { return data ? data : ""; }
   //const char *StrOrNull() const { return (data && length) ? data : NULL; }
   const int Len() const { return length; }

   void operator +=(const char *src) { Append(src); }
   void operator +=(const char  src) { Append(&src, 1); }
   void operator +=(const QString &src) { Append(src); }
   void Append(const QString &src) { Append(src.data, src.length); }

   QString &operator =(const char *src)
   {
      Clear();
      Append(src);
      return *this;
   }

   QString &operator =(QString &src)
   {
      Clear();
      Append(src.data, src.length);
      return *this;
   }

   // Take over contents of temporary variable.
   void TakeOver(QString &src)
   {
      Clear();
      // perform shallow copy
      data = src.data;
      length = src.length;
      bufferSize = src.bufferSize;
      // and prevent buffer from getting deleted
      src.data = NULL;
   }

   void Empty()
   {
      length = 0;
      if(data)
         data[0] = 0;
   }

   void Clear()
   {
      delete data;
      data = NULL;
      length = bufferSize = 0;
   }

   // Grow() -- expand the memory block to the requested size for new data.
   bool Grow(int newSize)
   {
      newSize++; // add byte for null terminator
      if(newSize <= (int)bufferSize)
         return true;

      newSize = (newSize + 31) & ~31;  // round up to multiple of 32 bytes
      char *tmp = (char*)realloc(data, newSize);   // allocate new buffer.  Data is copied for us.
      if(tmp == NULL)
         return false;
      bufferSize = _msize(data = tmp); // store new buffer size
      return true;
   }

   // This function is greedy; it will grab more memory than required.
   // One terminating byte is always added to the requested size.
   void Append(const char *src, int srcLength = -1)
   {
      if(srcLength == -1 && src != NULL)
         srcLength = (int)strlen(src);
      if(srcLength <= 0)
         return;
      q_size_t newSize = length + (q_size_t)srcLength + 1;
      if (newSize > bufferSize)
      {
          if (newSize < 32)
              newSize = 32;
          else if (newSize < 0x10000)
              newSize *= 2;
          else if (newSize < 0x100000)
              newSize += newSize / 10;
          Grow(newSize);
      }
      if (src == NULL)
         memset(data + length, 0, srcLength);
      else
         memcpy(data + length, src, srcLength);
      length += srcLength;
      data[length] = 0;
   }

   void Append(int num, int base=10)
   {
      char str[33];
      Append(_itoa(num, str, base));
   }

#if QSTRING_HEX
   void AppendHex(const char *src, int srcLength = -1)
   {
      if(srcLength == -1)
         srcLength = strlen(src);
      int hexLength = (srcLength + 1) >> 1; // two ASCII nibbles per hex byte
      Grow(length + hexLength);
      
      for(int i = 0; i < srcLength-1; i += 2)
         data[length++] = QHex::to_hex_rev(src+i);
      if(i < srcLength) // is there one byte left?
         data[length++] = QHex::to_hex(src[i]); // get the last nibble

      // We probably don't need a terminator when using binary data,
      data[length] = 0; // but let's put one in anyway.
   }
#endif // QSTRING_HEX

   void Reverse() // we can do better than strrev() because we know length
   {
      char tmp, *begin = data, *end = data + length - 1;
      while(begin < end)
      {
         tmp = *begin;
         *begin = *end;
         begin++;
         *end = tmp;
         end--;
      }
   }

   char At(int index) const
   {
      if (index >= 0 && index < (int)length)
         return data[index];
      else if (index < 0 && index >= -(int)length)
         return data[length + index];
      else
         return 0;
   }

   int Find(const char *str, int len = -1, size_t offset = 0)
   {
      if (!str) return -1;       // can't search for NULL
      if (len == -1) len = (int)strlen(str);
      if (!len) return -1;       // can't search for empty string
      int i;

      while (offset < (size_t)length)
      {
         for (i = 0; i < len && data[offset+i] == str[i]; i++)
            ;
         if (i == len)
            return (int)offset;
         offset++;
      }
      return -1;
   }

   int Replace(const char *src, const char *dst)
   {
      // Warning: This doesn't handle embedded nulls.
      if (!src || !dst)
         return -1;
      int srclen = (int)strlen(src), dstlen = (int)strlen(dst);
      if (srclen == 0)
         return -1;
      int i = 0, count = 0;
      /*if (dstlen == srclen)
      {
         while (i < length)
         {
            if (!strncmp(data+i, src, srclen))
            {
               count++;
               memcpy(data+i, dst, dstlen);
               i += srclen;
            }
            else
               i++;
         }
      }
      else*/
      {
         QString tmp(length);
         int next;
         while((next = Find(src, srclen, i)) >= 0)
         {
            count++;
            tmp.Append(data + i, next - i);
            tmp.Append(dst, dstlen);
            i = next + srclen;
         }
         tmp.Append(data + i, length - i);

         TakeOver(tmp); // swap without copying
      }
      return count;
   }

   int Replace(char src, char dst)
   {
      int count = 0;
      for (int i = 0; i < (int)length; i++)
      {
         if (data[i] == src)
         {
            count++;
            data[i] = dst;
         }
      }
      return count;
   }

   int Printf(const char *fmt, ...)
   {
      va_list args;
      va_start(args, fmt);
      Empty();
      int count = _vscprintf(fmt, args);
      if (count < 0)
      {
          data[0] = 0;
          return -1;
      }
      Grow(count);
      va_start(args, fmt);
      _vsnprintf(data, bufferSize, fmt, args);
      data[count] = 0;
      return length = count;
   }

   void SubstituteVariables()
   {
     int start = 0, last = 0;
     QString tmp;
     while (1)
     {
        start = Find("%", 1, start);
        if (start < 0)
           break;
        int end = Find("%", 1, start + 1);
        if (end < 0)
        {
           start = -1;
           break;
        }
        QString varname(data + start + 1, end - start - 1);
        DWORD vlen = GetEnvironmentVariableA(varname, NULL, 0);
        if (vlen)
        {
           tmp.Append(data + last, start - last);
           tmp.Grow(tmp.Len() + vlen);
           GetEnvironmentVariableA(varname, tmp.data + tmp.Len(), vlen + 1);
           tmp.length += vlen - 1; // vlen includes null terminator
           last = end + 1;
        }
        start = end + 1;
      }
      if (start < 0)
          start = Len();
      tmp.Append(data + last, start - last);
      TakeOver(tmp); // *this = tmp, but without the extra malloc
   }
};

#endif // _QSTRING_H_
