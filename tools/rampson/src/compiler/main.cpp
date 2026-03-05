#include "compiler.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: Rampsonc <schema_file.json>" << std::endl;
    return 2;
  }

  std::filesystem::path inputPath; 
  std::filesystem::path outputPath;
  std::string namespaceName = "JSchema";
  try {
    inputPath = argv[1];
    if (!std::filesystem::exists(inputPath)) {
        std::cerr << "Error: Input file does not exist: " << inputPath << std::endl;
        return 1;
    }
    if (!std::filesystem::is_regular_file(inputPath)) {
        std::cerr << "Error: Path is not a regular file: " << inputPath << std::endl;
        return 1;
    }
    outputPath = ("jsonSchema.hpp");
    if(argc >= 3) {
      outputPath = argv[2];
    }
    if(argc >= 4) {
      namespaceName = argv[3];
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
    return 1;
  }
  
  std::ifstream schemaFile(inputPath);
  if (!schemaFile.is_open()) {
    std::cerr << "Error: Could not open schema file: " << inputPath << std::endl;
    return 1;
  }
  std::ofstream outputFile(outputPath);
  if (!outputFile.is_open()) {
    std::cerr << "Error: Could not open output file: " << outputPath << std::endl;
    return 1;
  }

  json jsonSchema;
  try {
      jsonSchema = json::parse(schemaFile);
      std::cout << "Successfully parsed JSON" << std::endl;
  } catch (const json::parse_error& e) {
      std::cerr << "JSON Parse Error:" << std::endl;
      std::cerr << "  Message: " << e.what() << std::endl;
      std::cerr << "  Byte: " << e.byte << std::endl;
      return 1;
  } 

  outputFile << compile(jsonSchema, inputPath.filename().string(), namespaceName);

  schemaFile.close();
  outputFile.close();

  std::cout << "Compilation successful. Output written to " << outputPath << std::endl;

  return 0;
}