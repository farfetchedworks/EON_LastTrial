#pragma once

#include "resources/resources.h"

class CBTree;
class IBTTask;
class IBTDecorator;
class IBTService;
class CBTNode;
enum class EBTNodeType;

class CBTParser
{
public:
    // tasks
    class IBTTaskFactory
    {
    public:
        virtual IBTTask* create() = 0;
    };

    template <typename T>
    class CBTTaskFactory : public IBTTaskFactory
    {
    public:
      IBTTask* create() override { return new T(); }
    };

    // decorators
    class IBTDecoratorFactory
    {
    public:
        virtual IBTDecorator* create() = 0;
    };

    template <typename T>
    class CBTDecoratorFactory : public IBTDecoratorFactory
    {
    public:
      IBTDecorator* create() override { return new T(); }
    };

    // services
    class IBTServiceFactory
    {
    public:
        virtual IBTService* create() = 0;
    };

    template <typename T>
    class CBTServiceFactory : public IBTServiceFactory
    {
    public:
        IBTService* create() override { return new T(); }
    };

    static void registerTypes();
    static IBTTask* createTask(const std::string& type);
    static IBTDecorator* createDecorator(const std::string& type);
    static IBTService* createService(const std::string& type);

    static bool parse(CBTree* btree, CBTNode* parent, const json& j);

    static EBTNodeType stringToNodeType(const std::string& str);

private:
    static std::map<std::string_view, IBTTaskFactory*> task_types;
    static std::map<std::string_view, IBTDecoratorFactory*> decorator_types;
    static std::map<std::string_view, IBTServiceFactory*> service_types;
};
