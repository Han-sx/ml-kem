#ifndef CUSTOM_FILLER_HPP
#define CUSTOM_FILLER_HPP

#include <fstream>  // For file input
#include <array>    // For std::array
#include <iostream> // For printing
#include <cstring>  // For std::memcpy
#include <span>     // For std::span
#include <algorithm> // For std::min
#include <iomanip>  // For std::setw and std::setfill

template<typename UIntType = uint8_t>
class custom_filler_t
{
private:
  std::array<uint8_t, 32> buffer{};  // Define buffer size (example: 136 bytes)
  size_t buffer_offset = 0;

public:
  using result_type = UIntType;

  // Constructor to initialize buffer with custom data from a file
  explicit custom_filler_t(const std::string& file_path)
  {
    // Open the file for reading
    std::ifstream file(file_path, std::ios::binary);
    
    if (!file) {
      std::cerr << "Failed to open file: " << file_path << std::endl;
      return;
    }

    // Read data from file into buffer
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    if (static_cast<size_t>(file.gcount()) < buffer.size()) {
      std::cerr << "Warning: File data is smaller than the buffer size!" << std::endl;
    }

    file.close();
    buffer_offset = 0;
  }

  // Delete copy and move constructors - as this object is neither copyable nor movable.
  custom_filler_t(const custom_filler_t&) = delete;
  custom_filler_t(custom_filler_t&&) = delete;
  custom_filler_t& operator=(const custom_filler_t&) = delete;
  custom_filler_t& operator=(custom_filler_t&&) = delete;

  // Zeroize internal state when destroying an instance
  ~custom_filler_t()
  {
    buffer.fill(0);
    buffer_offset = 0;
  }

  // Returns a value of type `result_type` from the buffer (no random generation)
  result_type operator()()
  {
    constexpr size_t required_num_bytes = sizeof(result_type);
    const size_t readable_num_bytes = buffer.size() - buffer_offset;

    // Ensure the buffer is properly sized and we don't run out of data
    if (readable_num_bytes == 0) {
      buffer_offset = 0;  // Reset the buffer offset when exhausted
    }

    result_type result{};

    auto src_ptr = reinterpret_cast<const uint8_t*>(buffer.data()) + buffer_offset;
    auto dst_ptr = reinterpret_cast<uint8_t*>(&result);

    std::memcpy(dst_ptr, src_ptr, required_num_bytes);
    buffer_offset += required_num_bytes;

    return result;
  }

  // This function fills the output with the provided custom data
  void generate(std::span<uint8_t> output)
  {
    size_t out_offset = 0;

    while (out_offset < output.size()) {
      const size_t readable_num_bytes = buffer.size() - buffer_offset;
      const size_t required_num_bytes = output.size() - out_offset;
      const size_t copyable_num_bytes = std::min(readable_num_bytes, required_num_bytes);

      auto src_ptr = reinterpret_cast<const uint8_t*>(buffer.data()) + buffer_offset;
      auto dst_ptr = reinterpret_cast<uint8_t*>(output.data()) + out_offset;

      std::memcpy(dst_ptr, src_ptr, copyable_num_bytes);

      buffer_offset += copyable_num_bytes;
      out_offset += copyable_num_bytes;

      if (buffer_offset == buffer.size()) {
        buffer_offset = 0;  // Reset buffer_offset if it's exhausted
      }
    }
  }

  // Method to print the data read from the file
  void print_buffer() const
  {
    std::cout << "Buffer content (" << buffer.size() << " bytes): ";
    for (const auto& byte : buffer) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << "\n"; // Reset to decimal output
  }
 /*void print_buffer(const std::vector<uint8_t>& buffer) const {
    std::cout << "Buffer content (" << buffer.size() << " bytes): ";
    for (const auto& byte : buffer) {
        std::cout << std::dec << static_cast<int>(byte) << " ";  // 打印十进制
    }
    std::cout << "\n";
}*/
};

#endif // CUSTOM_FILLER_HPP
