// Scans BLE devices and prints the address and possible manufacturer data of the discovered devices.
// Continues scanning until interrupted or if options -a and -1 are defined, then exit after the
// specified devices have been found. Option -m can be used to only print information about devices
// that have manufacturer data with the defined identifier.
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <blepp/lescan.h>

using namespace std;
using namespace BLEPP;

bool has_manufacturer_data(AdvertisingResponse& ad, uint16_t manufacturer_id) {

    for (const auto& md: ad.manufacturer_specific_data) {
        uint16_t id = md[0] | (md[1] << 8);

        if (id == manufacturer_id) {
            return true;
        }
    }

    return false;
}

void parse_addresses(const char* arg, set<string>& dst) {
    char delimiter = ',';
    string token;
    istringstream token_stream(arg);

    while (getline(token_stream, token, delimiter)) {
        transform(token.begin(), token.end(), token.begin(), ::tolower);
        dst.insert(token);
    }
}

void catch_function(int) {
    cerr << "\nInterrupted!\n";
}

int main(int argc, char** argv)
{
    try {
        HCIScanner::ScanType type = HCIScanner::ScanType::Passive;
        HCIScanner::FilterDuplicates filter = HCIScanner::FilterDuplicates::Off;
        string device = "";
        set<string> addresses;
        bool loop = true;
        bool addresses_defined = false;
        int manufacturer_id = -1;

        int c;
        string help = R"X(
 -a  list of ble devices to scan, bluetooth addresses separated by comma
 -d  address of the bluetooth adapter to use
 -m  manufacturer id as a hex string, by default print any adv. data
 -1  exit after each specified device has been found, by default loops until interrupted
 -h  show this message

)X";

        while ((c = getopt(argc, argv, "a:m:d:1h")) != -1) {

            if (c == 'a') {
                parse_addresses(optarg, addresses);
                addresses_defined = !addresses.empty();
            } else if (c == 'm') {
                manufacturer_id = (int)strtol(optarg, NULL, 0);
            } else if (c == 'd') {
                device = string(optarg);
            } else if (c == '1') {
                loop = false;
            } else if (c == 'h') {
                cout << "Usage: " << argv[0] << " " << help;
                return 0;
            } else {
                cerr << argv[0] << ":  unknown option " << c << endl;
                return 1;
            }
        }

        log_level = LogLevels::Warning;
        HCIScanner scanner(true, filter, type, device);

        // Catch the interrupt signal. If the scanner is not cleaned up properly, then it doesn't reset the HCI state.
        signal(SIGINT, catch_function);

        bool done = false;

        while (!done) {
            // Check to see if there's anything to read from the HCI and wait if there's not.
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 300000;
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(scanner.get_fd(), &fds);
            int err = select(scanner.get_fd()+1, &fds, NULL, NULL,  &timeout);

            if (err < 0 && errno == EINTR) {
                // Interrupted, so quit and clean up properly.
                break;
            }

            if (FD_ISSET(scanner.get_fd(), &fds)) {
                vector<AdvertisingResponse> ads = scanner.get_advertisements();

                for (auto& ad: ads) {
                    bool address_pass = addresses.empty() || addresses.find(ad.address) != addresses.end();
                    bool md_pass = manufacturer_id < 0 || has_manufacturer_data(ad, manufacturer_id);

                    if (address_pass && md_pass) {
                        auto addr = string(ad.address);
                        transform(addr.begin(), addr.end(), addr.begin(), ::toupper);

                        cout << addr << ' ';

                        if (!ad.manufacturer_specific_data.empty()) {
                            auto md = ad.manufacturer_specific_data.front();

                            for (uint8_t b : md) {
                                // Without the '+' prefix integer always gets printed as a character,
                                // i.e. it is treated as ASCII code value. https://stackoverflow.com/a/28414758
                                cout << setfill('0') << setw(2) << hex << +b;
                            }
                        }

                        cout << endl;
                    }

                    if (!loop && addresses_defined && address_pass) {
                        addresses.erase(addresses.find(ad.address));
                        done = addresses.empty();
                    }
                }
            }
        }

    } catch (...) {
        cerr << "error occurred" << endl;
    }
}
