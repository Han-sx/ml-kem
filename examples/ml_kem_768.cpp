#include "ml_kem/ml_kem_768.hpp"
#include "randomshake/randomshake.hpp"
#include <algorithm>
#include <cassert>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

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

// Convert hex string to byte array
void
hexToBytes(const std::string& hex, CryptoPP::byte* bytes)
{
  CryptoPP::HexDecoder decoder;
  decoder.Put((const CryptoPP::byte*)hex.data(), hex.size());
  decoder.MessageEnd();

  CryptoPP::word64 size = decoder.MaxRetrievable();
  if (size && size <= CryptoPP::AES::DEFAULT_KEYLENGTH) {
    decoder.Get(bytes, size);
  }
}

// AES encryption function
std::string
aes_encrypt(const std::string& plaintext, const CryptoPP::byte* key, const CryptoPP::byte* iv)
{
  std::string ciphertext;

  CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
  encryption.SetKeyWithIV(key, CryptoPP::AES::DEFAULT_KEYLENGTH, iv);

  CryptoPP::StreamTransformationFilter stfEncryptor(encryption, new CryptoPP::StringSink(ciphertext));
  stfEncryptor.Put(reinterpret_cast<const CryptoPP::byte*>(plaintext.data()), plaintext.size());
  stfEncryptor.MessageEnd();

  return ciphertext;
}

// AES decryption function
std::string
aes_decrypt(const std::string& ciphertext, const CryptoPP::byte* key, const CryptoPP::byte* iv)
{
  std::string decryptedtext;

  CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
  decryption.SetKeyWithIV(key, CryptoPP::AES::DEFAULT_KEYLENGTH, iv);

  CryptoPP::StreamTransformationFilter stfDecryptor(decryption, new CryptoPP::StringSink(decryptedtext));
  stfDecryptor.Put(reinterpret_cast<const CryptoPP::byte*>(ciphertext.data()), ciphertext.size());
  stfDecryptor.MessageEnd();

  return decryptedtext;
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

  // Pseudo-randomness source
  randomshake::randomshake_t<192> csprng{};

  // Fill up seeds using PRNG
  csprng.generate(d_span);
  csprng.generate(z_span);

  // Generate a keypair
  ml_kem_768::keygen(d_span, z_span, pkey_span, skey_span);

  // Fill up seed required for key encapsulation, using PRNG
  csprng.generate(m_span);

  // Encapsulate key, compute cipher text and obtain KDF
  const bool is_encapsulated = ml_kem_768::encapsulate(m_span, pkey_span, cipher_span, sender_key_span);
  // Decapsulate cipher text and obtain KDF
  ml_kem_768::decapsulate(skey_span, cipher_span, receiver_key_span);

  // Check that both of the communicating parties arrived at same shared secret key
  assert(std::ranges::equal(sender_key_span, receiver_key_span));

  std::cout << "ML-KEM-768\n";
  std::cout << "Pubkey         : " << to_hex(pkey_span) << "\n";
  std::cout << "Seckey         : " << to_hex(skey_span) << "\n";
  std::cout << "Encapsulated ? : " << std::boolalpha << is_encapsulated << "\n";
  std::cout << "Cipher         : " << to_hex(cipher_span) << "\n";
  std::cout << "Shared secret  : " << to_hex(sender_key_span) << "\n";

  // Added AES algorithm

  // // Plaintext to be encrypted
  // std::string plaintext = "Power consumption side test data.";

  // Read file
  std::ifstream file("data_file.dat", std::ios::binary);
  if (!file) {
      std::cerr << "无法打开文件" << std::endl;
      return EXIT_FAILURE;
  }

  // Read files to plaintext
  std::string plaintext((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  // Generate AES key and initialization vector (IV) 256/128
  CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
  CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];

  // Convert hex key to byte array
  hexToBytes(to_hex(sender_key_span), key);

  // Here, IV is initialized to 0. In actual applications, a secure random number generator should be used.
  memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

  // Encrypted Plaintext
  std::string ciphertext = aes_encrypt(plaintext, key, iv);

  // Convert ciphertext to hex
  std::string ciphertextHex;
  CryptoPP::StringSource(ciphertext, true,
      new CryptoPP::HexEncoder(
          new CryptoPP::StringSink(ciphertextHex)
      )
  );

  // Save ciphertext hex to file
  std::ofstream outputFile("ciphertext.txt");
  if (outputFile.is_open()) {
      outputFile << ciphertextHex;
      outputFile.close();
      std::cout << "Ciphertext (hex) has been saved to ciphertext.txt" << std::endl;
  } else {
      std::cerr << "Unable to open file for writing ciphertext." << std::endl;
      return EXIT_FAILURE;
  }

  // 定义要执行的 Python 命令
  const char* command = "python3 Encrypt_data.py";

  // 使用 popen 打开一个管道来执行命令
  FILE* pipe = popen(command, "r");
  if (!pipe) {
      std::cerr << "无法执行命令" << std::endl;
      return -1;
  }

  // 用于存储命令输出的缓冲区
  char buffer[128];
  std::string result;

  // 从管道中读取命令输出
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;
  }

  // 关闭管道
  int returnCode = pclose(pipe);

  // 检查命令执行结果
  if (returnCode == 0) {
      std::cout << "Python 脚本执行成功，输出结果如下：" << std::endl;
      std::cout << result << std::endl;
  } else {
      std::cerr << "Python 脚本执行失败，返回码: " << returnCode << std::endl;
  }

  // Read ciphertext hex from file
  std::ifstream readFile("ciphertext.txt");
  if (!readFile) {
      std::cerr << "无法打开 ciphertext.txt 文件" << std::endl;
      return EXIT_FAILURE;
  }
  std::string readCiphertextHex;
  readFile >> readCiphertextHex;
  readFile.close();

  // Convert hex to bytes
  std::string readCiphertext;
  CryptoPP::StringSource(readCiphertextHex, true,
      new CryptoPP::HexDecoder(
          new CryptoPP::StringSink(readCiphertext)
      )
  );

  // Decrypting ciphertext
  std::string decryptedtext = aes_decrypt(readCiphertext, key, iv);

  // Output
  std::cout << "Plaintext: " << plaintext << std::endl;
  std::cout << "Ciphertext (hex): ";
  for (unsigned char c : ciphertext) {
    printf("%02x", c);
  }
  std::cout << std::endl;
  std::cout << "Decrypted text: " << decryptedtext << std::endl;

  return EXIT_SUCCESS;
}
