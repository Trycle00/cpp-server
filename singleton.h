#ifndef TRY_SINGLETON_H
#define TRY_SINGLETON_H

template <class T, class X = void, int N = 0>
class Singleton
{
public:
    static T* GetInstance()
    {
        static T t;
        return &t;
    }

private:
    Singleton() = default;
};

template <class T, class X = void, int N = 0>
class SingletonPtr final
{
public:
    static std::shared_ptr<T> GetSingleton()
    {
        static std::shared_ptr<T> ptr = std::make_shared<T>();
        return ptr;
    }

private:
    SingletonPtr() = default;
};

#endif // TRY_SINGLETON_H