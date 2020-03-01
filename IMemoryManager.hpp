
class IMemoryManager
{
public:
        virtual void *allocate(const std::size_t&) = 0;
        virtual void free(void *) = 0;
};