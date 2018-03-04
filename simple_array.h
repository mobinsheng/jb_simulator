#ifndef SIMPLE_ARRAY_H
#define SIMPLE_ARRAY_H

template<typename T>
class SimpleArray{
public:
    SimpleArray(size_t capacity){
        data = NULL;
        this->capacity = capacity;
        size = 0;
        data = new T[capacity];
    }

    ~SimpleArray(){
        delete[] data;
    }

    void Push(T v){
        data[size++] = v;
    }

    T Pop(){
        T v = data[0];
        memmove(data,data + 1,sizeof(T) * (size - 1));
        --size;
        return v;
    }

    T* Data(){
        return data;
    }

    size_t Size(){
        return size;
    }

private:
    T* data;
    size_t capacity;
    size_t size;
};

#endif // SIMPLE_ARRAY_H
