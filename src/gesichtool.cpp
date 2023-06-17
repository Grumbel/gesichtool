#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include <opencv2/opencv.hpp>
#include <fmt/format.h>

namespace fs = std::filesystem;

void detectAndExtractFaces(cv::Mat const& inputImage, std::string const& outputDir)
{
  cv::CascadeClassifier faceCascade;
  faceCascade.load(cv::samples::findFile("haarcascades/haarcascade_frontalface_default.xml"));

  std::vector<cv::Rect> faces;
  faceCascade.detectMultiScale(inputImage, faces, 1.1, 3, 0, cv::Size(256, 256), cv::Size());

  std::cout << fmt::format("Detected {}\n", faces.size());

  int face_idx = 0;
  for (const auto& face : faces) {
    cv::Mat faceROI = inputImage(face);

    std::string const filename = fmt::format("{}/face{:03d}.jpg", outputDir, face_idx);

    cv::resize(faceROI, faceROI, cv::Size(512, 512));

    cv::imwrite(filename, faceROI);

    face_idx += 1;
  }
}

int main(int argc, char* argv[])
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <input_image1> <input_image2> ... <output_directory>" << std::endl;
    return 1;
  }

  std::string const outputDir = argv[argc - 1];
  fs::create_directory(outputDir);

  for (int i = 1; i < argc - 1; ++i)
  {
    cv::Mat inputImage = cv::imread(argv[i]);

    if (inputImage.empty()) {
      std::cerr << "Failed to read image: " << argv[i] << std::endl;
      continue;
    }

    detectAndExtractFaces(inputImage, outputDir);
  }

  return 0;
}

/* EOF */
