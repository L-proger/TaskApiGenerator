#include "TypeCache.h"
#include <filesystem>
#include <Generator/CSharp/CSharpGenerator.h>
#include <Generator/Cpp/CppGenerator.h>
#include <iostream>
#include <Generator/Guid.h>
#include <Generator/Attribute/AttributeUtils.h>

#include <CommandLine/Parser.h>

enum class OutputLanguage {
    Cpp,
    CSharp
};

template<>
class CommandLine::ValueConverter<OutputLanguage> {
public:
    static OutputLanguage convert(const std::string& value) {
        if(value == "cpp" || value == "Cpp" || value == "CPP"){
            return OutputLanguage::Cpp;
        }
        if(value == "CS" || value == "cs" || value == "Cs" || value == "csharp" || value == "CSharp"){
            return OutputLanguage::CSharp;
        }
        throw CommandLine::Exception("Failed to convert OutputLanguage value");
    }
};


static std::string moduleNameFromInterface(const std::string& interfaceName) {
    auto dotPos = interfaceName.find_last_of('.');
    return interfaceName.substr(0, dotPos);
}

static std::size_t writeOutMethods(std::shared_ptr<InterfaceType> type, std::shared_ptr<CppCodeFile> file) {
    std::size_t offset = 0;
    if (type->baseInterfaceType->type->name != "LFramework::IUnknown") {
        offset = writeOutMethods(std::dynamic_pointer_cast<InterfaceType>(type->baseInterfaceType->type), file);
    }

    for (std::size_t i = 0; i < type->methods.size(); ++i) {
        auto method = type->methods[i];

        auto globalMethodId = offset + i;
        file->write("void ").write(method.name).write("(");
        for (int j = 0; j < method.args.size(); ++j) {
            auto arg = method.args[j];
            file->write(file->fullName(arg.type)).write(" ").write(arg.name);
            if (1 < method.args.size() - 1) {
                file->write(", ");
            }
        }
        file->write(")");


        file->beginScope(method.name);
        file->write("//PacketID: ").writeLine(std::to_string(globalMethodId));

        file->writeLine("MicroNetwork::Common::PacketHeader header;");
        file->writeLine("header.id = " + std::to_string(globalMethodId) + ";");

        file->writeLine("header.size = sizeof(" + file->fullName(method.args[0].type) + ");");

        file->writeLine("_next->packet(header, &" + method.args[0].name + ");");

      /*
       
        _next->packet(header, &request);*/

        file->endScope();
    }

    return offset + type->methods.size();
}

void run(OutputLanguage language, const std::string& outputDir, const std::vector<std::string>& inputDirs, std::optional<bool> enableExceptions, std::string targetInterfaceName, bool isOutMarshaling) {
   

    TypeCache::init();
    for(auto& dir : inputDirs){
        TypeCache::addSearchPath(dir);
    }

    auto targetModuleName = moduleNameFromInterface(targetInterfaceName);

    auto shortInterfaceName = targetInterfaceName.substr(targetModuleName.size() + 1);

    auto targetModule = TypeCache::parseModule(targetModuleName);

    auto commonModule = TypeCache::parseModule("MicroNetwork.Common");

   /* for (auto& module : modules) {
        TypeCache::parseModule(module);
    }*/

    std::string suffix = (isOutMarshaling ? std::string("Out") : std::string("In")) + "Marshaler";

    auto resultModule = std::make_shared<Module>();
    resultModule->name = targetInterfaceName + "." + suffix;
    resultModule->importedTypes.insert(resultModule->importedTypes.end(), targetModule->importedTypes.begin(), targetModule->importedTypes.end());
   
   

    auto parsedModules = TypeCache::getModules();

    if(language == OutputLanguage::Cpp){
        CppGenerator generator;
        if (enableExceptions.has_value()) {
            generator.enableExceptions = enableExceptions.value();
        }
      

        auto itf =  targetModule->findType(shortInterfaceName);

        auto idatareceiver = commonModule->findType("IDataReceiver");

        resultModule->importedTypes.push_back(itf);
        resultModule->importedTypes.push_back(idatareceiver);

        auto codeFile = std::static_pointer_cast<CppCodeFile>( generator.createCodeFile()); 
        codeFile->writeModule(resultModule, false);

        codeFile->beginNamespaceScope(targetModule->name);

        std::string marshalerName = shortInterfaceName + suffix;

        //: public LFramework::ComImplement<IHostToDeviceMarshaler, LFramework::ComObject, MicroNetwork::Task::MemoryAccess::IHostToDevice> {
        codeFile->write("class ").write(marshalerName).write(" : public LFramework::ComImplement<").write(marshalerName).write(", LFramework::ComObject, ").write(codeFile->fullName(itf)).write(">");
        codeFile->beginScope(marshalerName);


        codeFile->unident().write("public:").ident().writeLine();


        auto itfType = std::dynamic_pointer_cast<InterfaceType>(itf->type);
        
        writeOutMethods(itfType, codeFile);
    
        codeFile->unident().write("protected:").ident().writeLine();
        codeFile->writeLine("LFramework::ComPtr<MicroNetwork::Common::IDataReceiver>  _next;");
        

        codeFile->endScope(";");
        codeFile->endScope();

        /*
        class IHostToDeviceMarshaler : public LFramework::ComImplement<IHostToDeviceMarshaler, LFramework::ComObject, MicroNetwork::Task::MemoryAccess::IHostToDevice> {
public:
    IHostToDeviceMarshaler(LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> next) : _next(next) {
        
    }
    void read(MicroNetwork::Task::MemoryAccess::MemoryRegion request) {
        MicroNetwork::Common::PacketHeader header;
        header.id = 0;
        header.size = sizeof(MicroNetwork::Task::MemoryAccess::MemoryRegion);
        _next->packet(header, &request);
    }
    void write(MicroNetwork::Task::MemoryAccess::MemoryBlob request) {
        MicroNetwork::Common::PacketHeader header;
        header.id = 1;
        header.size = sizeof(MicroNetwork::Task::MemoryAccess::MemoryBlob);
        _next->packet(header, &request);
    }
protected:
    LFramework::ComPtr<MicroNetwork::Common::IDataReceiver> _next;
};


*/
       // codeFile->wr

        generator.saveCodeFile(outputDir, resultModule->name, codeFile);
       // generator.generate(outputDir);
    }else{
        throw std::runtime_error("Target language not supported");
    }
}

int main(int argc, const char* const* argv) {
    try{
        auto version = CommandLine::OptionDescription("--version", "Print version", CommandLine::OptionType::NoValue).alias("-v");
        auto exceptions = CommandLine::OptionDescription("--exceptions", "Enable exceptions handling in generated files", CommandLine::OptionType::SingleValue).alias("-e");
        auto language = CommandLine::OptionDescription("--language", "Output files language", CommandLine::OptionType::SingleValue).alias("-l");
        auto outputDir = CommandLine::OptionDescription("--output", "Output files directory", CommandLine::OptionType::SingleValue).alias("-o");
        auto inputDirs = CommandLine::OptionDescription("--input",  "Input files directory", CommandLine::OptionType::MultipleValues).alias("-I");

        auto targetInterface = CommandLine::OptionDescription("--target", "IN interface", CommandLine::OptionType::SingleValue);
        auto isOutDirection = CommandLine::OptionDescription("--out", "Is OUT direction marshaling (to network)", CommandLine::OptionType::NoValue);

        CommandLine::Command app("TaskApiGenerator", "Generate Task API implementation helpers for MicroNetwork from .cidl files", [&](CommandLine::Command& cmd) {
            cmd.addHelpOption();
            auto& versionOpt = cmd.option(version);
            cmd.handler([&]() {
                if(versionOpt.isSet()){
                    std::cout << "0.1.0-alpha" << std::endl;
                }else{
                    std::cout << "No command selected!" << std::endl;
                }
            });
        });
        app.command("generate", "Generate interface files", [&](CommandLine::Command& cmd) {
            cmd.addHelpOption();
            auto& languageOpt = cmd.option(language);
            auto& outputDirOpt = cmd.option(outputDir);
            auto& inputDirsOpt = cmd.option(inputDirs);
            auto& exceptionsOpt = cmd.option(exceptions);

            auto& targetInterfaceOpt = cmd.option(targetInterface);
            auto& isOutDirectionOpt = cmd.option(isOutDirection);

            cmd.handler([&]() {
                run(languageOpt.value<OutputLanguage>(), outputDirOpt.value(), inputDirsOpt.values(), exceptionsOpt.valueOptional<bool>(), targetInterfaceOpt.value(), isOutDirectionOpt.isSet());
            });
        });

        CommandLine::Parser commandLineParser;
        commandLineParser.parse(argc, argv, app);
        return 0;
    }catch(const CommandLine::Exception& ex){
        std::cerr << "CommandLine error: " << ex.what() << std::endl;
        return -1000;
    }catch(const std::exception& ex){
        std::cerr << "Generator error: " << ex.what() << std::endl;
        return -1;
    }catch(...){
        std::cerr << "Unexpected error" << std::endl;
        return -2;
    }
}
