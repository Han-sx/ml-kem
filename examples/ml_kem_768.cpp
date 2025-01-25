#include "ml_kem/ml_kem_768.hpp"
#include "custom.hpp"
#include "randomshake/randomshake.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

// Given a bytearray of length N, this function converts it to human readable hex formatted string of length 2*N | N >= 0.
static inline std::string
to_hex(std::span<const uint8_t> bytes)
{
  std::stringstream ss;
  ss << std::hex;

  for (size_t i = 0; i < bytes.size(); i++) {
    ss << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(bytes[i]);
  }

  return ss.str();
}

// Used to output span data
void
print_g_in_span0(const std::span<const uint8_t>& g_in_span0)
{
  std::cout << "Decrypting Data: ";
  std::string output; // Used to accumulate all printed characters
  for (const auto& byte : g_in_span0) {
    // If it is a printable character, add it to the output string
    if (std::isprint(byte)) {
      output += static_cast<char>(byte);
    } else {
      break; // Stop output when a non-printing character is encountered
    }
  }
  std::cout << output << "\n"; // Output the complete string
}

// Compile it with
//
// g++ -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -I ./include -I ./sha3/include -I ./subtle/include/ examples/ml_kem_768.cpp
int
main()
{
  // Seeds required for keypair generation
  std::vector<uint8_t> d(ml_kem_768::SEED_D_BYTE_LEN, 0);
  std::vector<uint8_t> z(ml_kem_768::SEED_Z_BYTE_LEN, 0);

  auto d_span = std::span<uint8_t, ml_kem_768::SEED_D_BYTE_LEN>(d);
  auto z_span = std::span<uint8_t, ml_kem_768::SEED_Z_BYTE_LEN>(z);

  // Public/ private keypair
  std::vector<uint8_t> pkey(ml_kem_768::PKEY_BYTE_LEN, 0);
  std::vector<uint8_t> skey(ml_kem_768::SKEY_BYTE_LEN, 0);

  auto pkey_span = std::span<uint8_t, ml_kem_768::PKEY_BYTE_LEN>(pkey);
  auto skey_span = std::span<uint8_t, ml_kem_768::SKEY_BYTE_LEN>(skey);

  // Seed required for key encapsulation
  std::vector<uint8_t> m(ml_kem_768::SEED_M_BYTE_LEN, 0);
  std::vector<uint8_t> cipher(ml_kem_768::CIPHER_TEXT_BYTE_LEN, 0);

  auto m_span = std::span<uint8_t, ml_kem_768::SEED_M_BYTE_LEN>(m);
  auto cipher_span = std::span<uint8_t, ml_kem_768::CIPHER_TEXT_BYTE_LEN>(cipher);

  // Shared secret that sender/ receiver arrives at
  std::vector<uint8_t> sender_key(ml_kem_768::SHARED_SECRET_BYTE_LEN, 0);
  std::vector<uint8_t> receiver_key(ml_kem_768::SHARED_SECRET_BYTE_LEN, 0);

  auto sender_key_span = std::span<uint8_t, ml_kem_768::SHARED_SECRET_BYTE_LEN>(sender_key);
  auto receiver_key_span = std::span<uint8_t, ml_kem_768::SHARED_SECRET_BYTE_LEN>(receiver_key);

  // Initialize encrypted message m
  std::vector<uint8_t> decrypted_data(768, 0); // Initialized to 768 bytes, filled with 0
  auto decrypted_span = std::span<uint8_t, 768>(decrypted_data);

  // Initialization run time
  std::chrono::duration<double> encryption_duration;
  std::chrono::duration<double> decapsulate_duration;
  std::chrono::duration<double> all_duration;

  // Pseudo-randomness source
  randomshake::randomshake_t<192> csprng{};

  // Fill up seeds using PRNG
  csprng.generate(d_span);
  csprng.generate(z_span);

  // Generate a keypair
  ml_kem_768::keygen(d_span, z_span, pkey_span, skey_span);

  // ------------------------------------------------------------------------

  // Fill up seed required for key encapsulation, using PRNG
  // csprng.generate(m_span);

  // Encapsulate key, compute cipher text and obtain KDF
  // const bool is_encapsulated = ml_kem_768::encapsulate(m_span, pkey_span, cipher_span, sender_key_span);

  // Decapsulate cipher text and obtain KDF
  // ml_kem_768::decapsulate(skey_span, cipher_span, receiver_key_span);

  // Check that both of the communicating parties arrived at same shared secret key
  // assert(std::ranges::equal(sender_key_span, receiver_key_span));

  // ------------------------------------------------------------------------

  // Reading electricity consumption data
  std::ifstream file("data_file.dat", std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open file: data_file.dat\n";
  } else {
    // Read the file data into the buffer
    std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t file_size = file_data.size();

    // Fill `m_span` using data from file
    size_t offset = 0;
    size_t m_span_size = m_span.size();
    auto start = std::chrono::high_resolution_clock::now();
    // Loop to fill m_span with file data in chunks of size 3
    while (offset + m_span_size <= file_size) {
      std::memcpy(m_span.data(), &file_data[offset], m_span_size);
      offset += m_span_size;

      // Perform encryption/decryption here
      auto start1 = std::chrono::high_resolution_clock::now();
      const bool is_encapsulated = ml_kem_768::encapsulate(m_span, pkey_span, cipher_span);
      auto end1 = std::chrono::high_resolution_clock::now();
      encryption_duration += end1 - start1;
      auto start2 = std::chrono::high_resolution_clock::now();
      ml_kem_768::decapsulate(skey_span, cipher_span, decrypted_span);
      auto end2 = std::chrono::high_resolution_clock::now();
      decapsulate_duration += end2 - start2;
    }

    // Handle case when file data is less than 32 bytes
    if (offset < file_size) {
      // Fill remaining space in m_span with incremental values
      for (size_t i = offset; i < file_size; ++i) {
        m_span[i - offset] = file_data[i];
      }
      // Fill remaining space in m_span with 123 (0x7B)
      for (size_t i = file_size - offset; i < m_span_size; ++i) {
        m_span[i] = 1; // Filling with 1
      }

      // Perform encryption/decryption for the last chunk
      auto start1 = std::chrono::high_resolution_clock::now();
      const bool is_encapsulated = ml_kem_768::encapsulate(m_span, pkey_span, cipher_span);
      auto end1 = std::chrono::high_resolution_clock::now();
      encryption_duration += end1 - start1;
      auto start2 = std::chrono::high_resolution_clock::now();
      ml_kem_768::decapsulate(skey_span, cipher_span, decrypted_span);
      auto end2 = std::chrono::high_resolution_clock::now();
      decapsulate_duration += end2 - start2;
    }
    auto end = std::chrono::high_resolution_clock::now();
    all_duration = end - start;
  }

  // std::cout << "ML-KEM-768\n";
  std::cout << "Pubkey         : " << to_hex(pkey_span) << "\n";
  std::cout << "Seckey         : " << to_hex(skey_span) << "\n";
  // std::cout << "Encapsulated ? : " << std::boolalpha << is_encapsulated << "\n";
  // std::cout << "Cipher         : " << to_hex(cipher_span) << "\n";
  // std::cout << "Shared secret  : " << to_hex(sender_key_span) << "\n";
  std::cout << "encryption time: " << encryption_duration.count() << " seconds\n";
  std::cout << "decapsulate time: " << decapsulate_duration.count() << " seconds\n";
  std::cout << "all time: " << all_duration.count() << " seconds\n";
  print_g_in_span0(decrypted_span);

  return EXIT_SUCCESS;
}
