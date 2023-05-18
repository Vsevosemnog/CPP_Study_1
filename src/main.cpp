#include <iostream>
#include <vector>
#include <bitset>
#include <assert.h>
#include <fstream>

//Java-like object model
namespace ObjectModel
{
    enum class Wrapper: int8_t
    {
        PRIMITIVE=1,
        ARRAY,
        STRING,
        OBJECT
    };    

    enum class Type: int8_t
    {
        I8=1,
        I16,
        I32,
        I64,

        U8,
        U16,
        U32,
        U64,

        FLOAT,
        DOUBLE,

        BOOL
    };
    class Root
    {
    protected:
        std::string name;
        int16_t nameLength;
        int8_t wrapper;
        int32_t size;
        Root();  
    public:
        int32_t getSize();
        void setName(std::string);
        std::string getName();
        virtual void pack(std::vector<int8_t>*, int16_t*);
        //virtual void pack(); // = 0; //pure virtual function 
        
    };
    class Primitive : public Root
    {
    private:
        int8_t type = 0;
        std::vector<int8_t>* data = nullptr;
    private:
        Primitive();
    public:
        //abstract factory? builder method?
        //static Primitive* createI32(std::string name, Type type, int32_t value);
        //static Primitive* createI64(std::string name, Type type, int64_t value);
        void pack(std::vector<int8_t>*, int16_t*);
        template<typename T>
        static Primitive* create(std::string name, Type type, T value)
        {
            Primitive* p = new Primitive();

            p->setName(name);
            p->wrapper= static_cast<int8_t>(Wrapper::PRIMITIVE);
            p->type=static_cast<int8_t>(type);
            p->data = new std::vector<int8_t>(sizeof value);
            int16_t iterator = 0;
            Core::template encode(p->data, &iterator, value);


            return p;
        }

    };
    class Array : public Root
    {

    };
    class Object : public Root
    {

    };
}


namespace Core
{
    namespace Util
    {
        bool isLittleEndian()
        {
            //0x00 0x00 0x00 0b0000 0101
            int a = 5;
            std::string result = std::bitset<0>(a).to_string();
            if (result.back() == '1') return true;
        }

        void save(const char* file, std::vector<int8_t> buffer)
        {
            std::ofstream out;
            out.open(file);

            for(unsigned i = 0; i < buffer.size(); i++)
            {
                out << buffer[i];
            }
            out.close();
        }

        void retriveNsave(ObjectModel::Root* r)
        {
            int16_t iterator = 0;
            std::vector<int8_t> buffer(r->getSize());
            std::string name = r->getName().substr(0, r->getName().length()).append(".ttc");
            //std::string name = r->getName().append(".ttc");
            r->pack(&buffer,&iterator);
            save(name.c_str(), buffer);

        }

    }
    // 0 1 2 3
    // 0x00 0x00 0x00 0x5 >> LE
    template <typename T>
    void encode(std::vector<int8_t>* buffer, int16_t* iterator, T value)
    {
        for (unsigned i = 0; i < sizeof(T) ; i++)
        {
            //(*buffer)[(*iterator)++] = value  >> ((sizeof(T) * 8) - 8 - ((i == 0) ? j : j += 8)); 
            //(*buffer)[(*iterator)++] = value  >> ((sizeof(T) * 8) - 8 - i*8); 
            (*buffer)[(*iterator)++] = value >> 8 * (sizeof(T) - i); 
        }

    } 

    template<>
    void encode<float>(std::vector<int8_t>* buffer, int16_t* iterator, float value)
    {
        int32_t result = *reinterpret_cast<int32_t*>(&value);
        encode<int32_t>(buffer,iterator, result);
    }

    template<>
    void encode<double>(std::vector<int8_t>* buffer, int16_t* iterator, double value)
    {
        int64_t result = *reinterpret_cast<int64_t*>(&value);
        encode<int64_t>(buffer,iterator, result);
    }

    template<>
    void encode(std::vector<int8_t>* buffer, int16_t* iterator,std::string value)
    { 
        for (unsigned i = 0 ; i <  value.size(); i++)
        {
            encode<int8_t>(buffer, iterator, value[i]);
        }
    } 
    template<typename T>
    void encode(std::vector<int8_t>* buffer, int16_t* iterator, std::vector<T> value)
    {
        for (unsigned i = 0; i < value.size(); i++)
        {
            encode<T>(buffer, iterator, value[i]);
        }
    }

/*     void encode16(std::vector<int8_t>* buffer, int16_t* iterator, int16_t value)
    {
        (*buffer)[(*iterator)++] = value >> 8;
        (*buffer)[(*iterator)++] = value;
    } */

}

namespace ObjectModel
{
    //definition

    Root::Root()
        :
        name("unknown"),
        wrapper(0),
        nameLength(0),
        size(sizeof nameLength + sizeof wrapper + sizeof size) {}

    void Root::setName(std::string name)
    {
        this->name = name;
        this->nameLength = static_cast<int16_t>(name.length());
        size += nameLength;
    }

    int32_t Root::getSize()
    {
        return size;
    }

    void Root::pack(std::vector<int8_t>*, int16_t*)
    {
        //
    }

    std::string Root::getName()
    {
        return name;
    }

    Primitive::Primitive()
    {
        size += sizeof type;
    }

    void Primitive::pack(std::vector<int8_t>* buffer, int16_t* iterator)
    {
        Core::encode<std::string>(buffer, iterator, name);
        Core::encode<int16_t>(buffer, iterator, nameLength);
        Core::encode<int8_t>(buffer,iterator,wrapper);
        Core::encode<int8_t>(buffer,iterator,type);
        Core::encode<int8_t>(buffer,iterator,*data);
        Core::encode<int32_t>(buffer,iterator,size);
    }
}


namespace EventSystem
{
    class Event;
    class System
    {
    private:
        friend class Event;
        std::string name;
        int32_t descriptor;  
        int16_t index;
        bool active;
        std::vector<Event*> events;
    public:
        System(std::string);
        ~System();
        void addEvent(Event*);
        Event* getEvent();
        void serialize();
    };

    class Event
    {
    public:

        enum DeviceType: int8_t
        {
            KEYBOARD = 1,
            MOUSE,
            TOUCHPAD,
            JOYSTICK
        };
        Event(DeviceType);
        DeviceType dType;
        System* system = nullptr;
        DeviceType getdType();
        friend std::ostream& operator<<(std::ostream& stream, const DeviceType dType)
        {
            std::string result;
#define PRINT(a) result = #a; // stringify
            switch(dType)
            {
                case KEYBOARD: PRINT(KEYBOARD); break;
                case MOUSE: PRINT(MOUSE); break;
                case TOUCHPAD: PRINT(TOUCHPAD); break;
                case JOYSTICK: PRINT(JOYSTICK); break;
            }
            return stream << result;
        }
        void bind(System*, Event*);
    };
    class KeyboardEvent : public Event
    {
    private:
        int16_t keyCode;
        bool pressed;
        bool released;
    public:
        KeyboardEvent(int16_t, bool, bool);
    };

    System::System(std::string name)
        :
        name(name),
        descriptor(123),
        index(1),
        active(true) {}
    
    System::~System()
    {
        // Todo
    }

    void System::addEvent(Event* e)
    {
        e->bind(this,e);
    }

    void System::serialize()
    {
        
    }
    Event* System::getEvent()
    {
        return events.front();
    }

    Event::Event(DeviceType dtype)
    {
        this->dType = dType;
    }

    void Event::bind(System* system, Event* e)
    {
        this->system=system;
        this->system->events.push_back(e);
    }

    Event::DeviceType Event::getdType()
    {
        return this->dType;
    }

    KeyboardEvent::KeyboardEvent(int16_t keyCode, bool pressed, bool released)
        :
        Event(Event::KEYBOARD),
        keyCode(keyCode),
        pressed(pressed),
        released(released) {}
}


using namespace EventSystem;
using namespace ObjectModel;
using namespace Core;

int main(int argc, char** argv)
{
    assert(Core::Util::isLittleEndian());
    int32_t foo = 5;
    Primitive* p = Primitive::create("int32", Type::I32, foo);
#if 0
    System foo("foo");
    Event* e = new KeyboardEvent('a',true, false);
    //Event* e1 = new KeyboardEvent('b',true, false);

    foo.addEvent(e);
    
    KeyboardEvent* kb = static_cast<KeyboardEvent*>(foo.getEvent());
    foo.serialize();
    //foo.addEvent(e1);
#endif
    (void)argc;
    (void)argv;
    return 0;
}