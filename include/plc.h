#ifndef PLC_H
#define PLC_H

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

class PLC {
public:
  PLC(const std::string &port, unsigned int baudrate, unsigned int timeout_ms);
  ~PLC();

  void Clear();
  void SendResults(int pos, bool final_status);
  void Send(const std::vector<uint8_t> &data);
  std::vector<uint8_t> Receive(std::size_t size);
  std::vector<uint8_t> ReadWithEnd(uint8_t end_signal);
  void Logout();
  std::string GetComport() const;
  
  static std::vector<std::string> GetListPort();

private:
  boost::asio::io_service io;
  boost::asio::serial_port serial;
  std::string port_name;
  unsigned int timeout;

  const uint8_t HEAD_RESPONSE = 0x01;
  const uint8_t TAIL_RESPONSE = 0x0A;
  const std::vector<uint8_t> POSITIONS = {0x02, 0x04};
  const std::map<int, uint8_t> RESULT_MAP = {
      {1, 0x01}, {2, 0x02}, {3, 0x03}, {4, 0x04}};
};

#endif
