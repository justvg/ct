#pragma once

template <typename T>
class DynamicArray
{
public:
    T* Elements;
    size_t Size;
    size_t Capacity;

public:
    DynamicArray();
    DynamicArray(size_t CapacityInit);
    DynamicArray(const DynamicArray<T>& Other);

    size_t SizeBytes();
    size_t CapacityBytes();

    void Push(T Element);
    void Resize(size_t NewSize);
    void Insert(T* First, size_t Count);
    void Free();

    T& operator[](size_t Index) const;
    DynamicArray<T>& operator=(const DynamicArray<T>& Other);

private:
    void ExpandMemory(size_t RequestedCapacity = 0);
};

template <typename T>
DynamicArray<T>::DynamicArray()
{
    Elements = 0;
    Size = Capacity = 0;
}

template <typename T>
DynamicArray<T>::DynamicArray(size_t CapacityInit)
{
    Size = Capacity = CapacityInit;
    
    if (CapacityInit > 0)
    {
        Elements = (T*)malloc(CapacityInit * sizeof(T));   
        Assert(Elements);
        memset(Elements, 0, CapacityBytes());
    }
    else
    {
        Elements = 0;
    }
}

template <typename T>
DynamicArray<T>::DynamicArray(const DynamicArray<T>& Other)
{
    Elements = 0;
    Size = Capacity = 0;
    
    Insert(Other.Elements, Other.Size);
}

template <typename T>
size_t DynamicArray<T>::SizeBytes()
{
    return Size * sizeof(T);
}

template <typename T>
size_t DynamicArray<T>::CapacityBytes()
{
    return Capacity * sizeof(T);
}

template <typename T>
void DynamicArray<T>::Push(T Element)
{
    if (Size >= Capacity)
    {
        ExpandMemory();
        Assert(Size < Capacity);
    }

    Elements[Size++] = Element;
}

template <typename T>
void DynamicArray<T>::Resize(size_t NewSize)
{
    if ((NewSize > Size) && (NewSize > Capacity))
    {
        ExpandMemory(NewSize);
    }

    Size = NewSize;
}

template <typename T>
void DynamicArray<T>::Insert(T* First, size_t Count)
{
    if (Size + Count > Capacity)
    {
        ExpandMemory(Size + Count);
        Assert(Size + Count <= Capacity);
    }

    for (size_t I = 0; I < Count; I++)
    {
        Elements[Size++] = First[I];
    }
}


template <typename T>
void DynamicArray<T>::Free()
{
    if (Elements)
    {
        free(Elements);
    }

    Size = Capacity = 0;
    Elements = 0;    
}

template <typename T>
T& DynamicArray<T>::operator[](size_t Index) const
{
    Assert(Index < Size);
    return Elements[Index];
}

template <typename T>
DynamicArray<T>& DynamicArray<T>::operator=(const DynamicArray<T>& Other)
{
    Free();
    Insert(Other.Elements, Other.Size);

    return *this;
}

template <typename T>
void DynamicArray<T>::ExpandMemory(size_t RequestedCapacity)
{
    size_t NewCapacity = (Capacity > 0) ? Capacity*2 : 1;
    if (RequestedCapacity != 0)
    {
        Assert(RequestedCapacity > Capacity);
        NewCapacity = RequestedCapacity;
    }

    T* NewMemory = (T*) malloc(NewCapacity * sizeof(T));
    Assert(NewMemory);

    memcpy(NewMemory, Elements, sizeof(T)*Size);
    memset(NewMemory + Size, 0, sizeof(T)*NewCapacity - sizeof(T)*Size);

    Capacity = NewCapacity;
    if (Elements)
    {
        free(Elements);
    }
    Elements = NewMemory;
}