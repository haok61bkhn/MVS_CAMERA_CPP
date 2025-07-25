#include "plc.h"
#include <algorithm>

PLC::PLC(const std::string &port, unsigned int baudrate,
         unsigned int timeout_ms)
    : serial(io), port_name(port), timeout(timeout_ms) {
  serial.open(port_name);
  serial.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
  serial.set_option(boost::asio::serial_port_base::character_size(8));
  serial.set_option(boost::asio::serial_port_base::stop_bits(
      boost::asio::serial_port_base::stop_bits::one));
  serial.set_option(boost::asio::serial_port_base::parity(
      boost::asio::serial_port_base::parity::none));
  serial.set_option(boost::asio::serial_port_base::flow_control(
      boost::asio::serial_port_base::flow_control::none));
}

PLC::~PLC() { Logout(); }

void PLC::Clear() { serial.cancel(); }

void PLC::SendResults(int pos, bool final_status) {
  if (final_status)
    return;

  std::vector<uint8_t> data;
  if (pos == 1) {
    data = {0x02, 0x01, 0x0A};
  } else {
    data = {0x04, 0x01, 0x0A};
  }
  Send(data);
}

void PLC::Send(const std::vector<uint8_t> &data) {
  boost::asio::write(serial, boost::asio::buffer(data));
}

std::vector<uint8_t> PLC::Receive(std::size_t size) {
  std::vector<uint8_t> buffer(size);
  boost::asio::read(serial, boost::asio::buffer(buffer, size));
  return buffer;
}

std::vector<uint8_t> PLC::ReadWithEnd(uint8_t end_signal) {
  boost::asio::streambuf buf;
  boost::asio::read_until(serial, buf, end_signal);
  const auto *data = boost::asio::buffer_cast<const uint8_t *>(buf.data());
  return std::vector<uint8_t>(data, data + buf.size());
}

void PLC::Logout() {
  if (serial.is_open())
    serial.close();
}

std::string PLC::GetComport() const { return port_name; }

std::vector<std::string> PLC::GetListPort() {
  std::vector<std::string> ports;
  boost::filesystem::path dev_path("/dev");
  
  if (boost::filesystem::exists(dev_path) && boost::filesystem::is_directory(dev_path)) {
    for (const auto& entry : boost::filesystem::directory_iterator(dev_path)) {
      std::string filename = entry.path().filename().string();
      
      if (filename.substr(0, 6) == "ttyUSB" || 
          filename.substr(0, 6) == "ttyACM" || 
          filename.substr(0, 4) == "ttyS") {
        ports.push_back(entry.path().string());
      }
    }
  }
  
  std::sort(ports.begin(), ports.end());
  return ports;
}
