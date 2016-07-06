#ifndef QVULKANBUFFER_H
#define QVULKANBUFFER_H
#include <vulkan/vulkan.h>
#include <qvkutil.h>
/*
class QVkDeviceMemory: public QVkDeviceResource
{
public:
    QVkDeviceMemory(VkDevice device)
        : QVkDeviceResource(device)
    {
    }

    void alloc(VkDeviceSize size, uint32_t typeIndex) {
        m_size = size;
        VkMemoryAllocateInfo allocinfo = {};
        allocinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocinfo.allocationSize = m_size;
        allocinfo.memoryTypeIndex = typeIndex;

        VkResult err = vkAllocateMemory(m_device, &allocinfo, nullptr, &m_memory);
        Q_ASSERT(!err);
    }

    ~QVkDeviceMemory() {
        if(m_size)
            vkFreeMemory(m_device, m_memory, nullptr);
        m_size = 0;
    }

    void* map() { return map(0, m_size); }

    void* map(VkDeviceSize offset, VkDeviceSize size) {
        Q_ASSERT(offset + size <= m_size);
        void* ptr;
        VkMemoryMapFlags flags {0};
        vkMapMemory(m_device, m_memory, offset, size, flags, &ptr);
        return ptr;
    }

    void unmap() {
        vkUnmapMemory(m_device, m_memory);
    }

    VkDeviceSize size() {
        return m_size;
    }

protected:
    VkDeviceSize m_size { 0 };
    VkDeviceMemory m_memory { nullptr };
};
*/

template <typename UniformStruct>
class QVkUniformBuffer :public QVkDeviceResource {
public:
    QVkUniformBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties* mem_props)
        : QVkDeviceResource(device)
        , m_memory_properties(mem_props)
    {
        DEBUG_ENTRY;
        VkResult err;
        VkBufferCreateInfo buf_ci = {};
        buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf_ci.size = sizeof(UniformStruct);
        err = vkCreateBuffer(m_device, &buf_ci, nullptr, &m_buffer);
        Q_ASSERT(!err);

        VkMemoryRequirements mem_reqs = {};
        vkGetBufferMemoryRequirements(m_device, m_buffer, &mem_reqs);

        VkMemoryAllocateInfo mem_ai;
        mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_ai.pNext = nullptr;
        mem_ai.allocationSize = mem_reqs.size;
        mem_ai.memoryTypeIndex = 0;


        bool pass = memory_type_from_properties(
           mem_reqs.memoryTypeBits,
           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
           &mem_ai.memoryTypeIndex);

        Q_ASSERT(pass);

        err = vkAllocateMemory(m_device, &mem_ai, nullptr, &m_memory);
        m_allocationSize = mem_ai.allocationSize;
        Q_ASSERT(!err);


        err = vkBindBufferMemory(m_device, m_buffer, m_memory, /*offset*/ 0);
        Q_ASSERT(!err);

        m_descriptorInfo.buffer = m_buffer;
        m_descriptorInfo.offset = 0;
        m_descriptorInfo.range = sizeof(UniformStruct);

    }
    ~QVkUniformBuffer() {
        DEBUG_ENTRY;
        vkDestroyBuffer(m_device, m_buffer, nullptr);
        vkFreeMemory(m_device, m_memory, nullptr);
    }

    UniformStruct* map() {
        DEBUG_ENTRY;
        VkResult err;
        UniformStruct* mappedAddr;
        err = vkMapMemory(m_device, m_memory,
                          /*offset*/ 0,
                          m_allocationSize,
                          /*flags*/0,
                          (void **)&mappedAddr);
        Q_ASSERT(!err);
        return mappedAddr;
    }

    void unmap() {
        DEBUG_ENTRY;
        vkUnmapMemory(device(), m_memory);
    }

    void update() {
        DEBUG_ENTRY;
        UniformStruct* deviceData = map();
        memcpy(deviceData, m_hostData, sizeof(UniformStruct) );
        unmap();
    }

    UniformStruct* data() {
        return &m_hostData;
    }

    bool memory_type_from_properties(uint32_t typeBits,
                                     VkFlags requirements_mask,
                                     uint32_t *typeIndex) {
        // Search memtypes to find first index with those properties
        for (uint32_t i = 0; i < 32; i++) {
            if ((typeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((m_memory_properties->memoryTypes[i].propertyFlags &
                     requirements_mask) == requirements_mask) {
                    *typeIndex = i;
                    return true;
                }
            }
            typeBits >>= 1;
        }
        // No memory types matched, return failure
        return false;
    }

    VkDescriptorBufferInfo* descriptorInfo() {
        DEBUG_ENTRY;
        return &m_descriptorInfo;
    }

protected:
    VkBuffer m_buffer { };
    VkDeviceMemory m_memory { };

    VkDeviceSize m_allocationSize { 0 };
    VkPhysicalDeviceMemoryProperties* m_memory_properties { nullptr };

    UniformStruct m_hostData { };

    VkDescriptorBufferInfo m_descriptorInfo { };

/*    class Accessor { //FIXME something about move semantics
    public:
        Accessor(QVkUniformBuffer& ubuf)
            : m_ubuf(ubuf) {
            m_mappedAddr = m_ubuf.map();
        }

        ~Accessor() {
            m_ubuf.unmap();
        }

        operator UniformStruct*() {
            return m_mappedAddr;
        }

    private:
        Accessor(Accessor&) = delete;
        Accessor& operator=(Accessor&) = delete; // fixme make copyable
        QVkUniformBuffer& m_ubuf;
        UniformStruct* m_mappedAddr;
    };*/
};


#endif // QVULKANBUFFER_H
