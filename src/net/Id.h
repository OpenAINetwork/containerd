#ifndef __ID_H__
#define __ID_H__


#include <string.h>


namespace containerd {


class Id
{
public:
    inline Id() :
        m_data()
    {
    }


    inline Id(const char *id, size_t sizeFix = 0)
    {
        setId(id, sizeFix);
    }


    inline bool operator==(const Id &other) const
    {
        return memcmp(m_data, other.m_data, sizeof(m_data)) == 0;
    }


    inline bool operator!=(const Id &other) const
    {
        return memcmp(m_data, other.m_data, sizeof(m_data)) != 0;
    }


    Id &operator=(const Id &other)
    {
        memcpy(m_data, other.m_data, sizeof(m_data));

        return *this;
    }


    inline bool setId(const char *id, size_t sizeFix = 0)
    {
        memset(m_data, 0, sizeof(m_data));
        if (!id) {
            return false;
        }

        const size_t size = strlen(id);
        if (size >= sizeof(m_data)) {
            return false;
        }

        memcpy(m_data, id, size - sizeFix);
        return true;
    }


    inline const char *data() const { return m_data; }
    inline bool isValid() const     { return *m_data != '\0'; }


private:
    char m_data[64];
};


}

#endif
