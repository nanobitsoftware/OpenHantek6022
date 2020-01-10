// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QObject>
#include <QStringList>
#ifdef __FreeBSD__
	#include <libusb.h>
#else
	#include <libusb-1.0/libusb.h>
#endif
#include <memory>

#include "usbdevicedefinitions.h"

class DSOModel;

typedef uint64_t UniqueUSBid;


/// \brief Returns string representation for libusb errors.
/// \param error The error code.
/// \return String explaining the error.
QString libUsbErrorString(int error);

/// \brief This class handles the USB communication with an usb device that has
/// one in and one out endpoint.
class USBDevice : public QObject {
    Q_OBJECT

  public:
    explicit USBDevice(DSOModel* model, libusb_device *device, unsigned findIteration = 0);
    USBDevice(const USBDevice&) = delete;
    ~USBDevice();
    bool connectDevice(QString &errorMessage);
    void disconnectFromDevice();

    /// \brief Check if the oscilloscope is connected.
    /// \return true, if a connection is up.
    bool isConnected();

    /**
     * @return Return true if this device needs a firmware first
     */
    bool needsFirmware();

    /**
     * @return Return device version as unsigned int
     */
    inline unsigned int getFwVersion() const { return this->descriptor.bcdDevice; }

    /**
     * Keep track of the find iteration on which this device was found
     * @param iteration The new iteration value
     */
    inline void setFindIteration(unsigned iteration) { findIteration = iteration; }
    inline unsigned getFindIteration() const { return findIteration; }

    /// \brief Bulk transfer to/from the oscilloscope.
    /// \param endpoint Endpoint number, also sets the direction of the transfer.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \param timeout The timeout in ms.
    /// \return Number of transferred bytes on success, libusb error code on
    /// error.
    int bulkTransfer(unsigned char endpoint, const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS,
                     unsigned int timeout = HANTEK_TIMEOUT);

    /// \brief Bulk write to the oscilloscope.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of sent bytes on success, libusb error code on error.
    inline int bulkWrite(const unsigned char *data, unsigned int length, int attempts = HANTEK_ATTEMPTS) {
         return bulkTransfer(HANTEK_EP_OUT, data, length, attempts);
    }

    /// \brief Bulk read from the oscilloscope.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    template<class T>
    inline int bulkRead(const T *command, int attempts = HANTEK_ATTEMPTS) {
        return bulkTransfer(HANTEK_EP_IN, command->data(), command->size(), attempts);
    }

    /// \brief Multi packet bulk read from the oscilloscope.
    /// \param data Buffer for the sent/received data.
    /// \param length The length of data contained in the packets.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of received bytes on success, libusb error code on error.
    int bulkReadMulti(unsigned char *data, unsigned length, int attempts = HANTEK_ATTEMPTS_MULTI);

    /// \brief Control transfer to the oscilloscope.
    /// \param type The request type, also sets the direction of the transfer.
    /// \param request The request field of the packet.
    /// \param data Buffer for the sent/received data.
    /// \param length The length field of the packet.
    /// \param value The value field of the packet.
    /// \param index The index field of the packet.
    /// \param attempts The number of attempts, that are done on timeouts.
    /// \return Number of transferred bytes on success, libusb error code on error.
    int controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length, int value,
                        int index, int attempts = HANTEK_ATTEMPTS);

    /// \brief Control write to the oscilloscope.
    /// \param command Buffer for the sent/received data.
    /// \return Number of sent bytes on success, libusb error code on error.
    template<class T>
    inline int controlWrite(const T *command) {
        return controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT, uint8_t(command->code),
                               const_cast<unsigned char *>(command->data()), unsigned(command->size()), command->value, 0,
                               HANTEK_ATTEMPTS);
    }

    /// \brief Control read to the oscilloscope.
    /// \param command Buffer for the sent/received data.
    /// \return Number of received bytes on success, libusb error code on error.
    template<class T>
    inline int controlRead(const T *command) {
        return controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, uint8_t(command->code),
                               const_cast<unsigned char *>(command->data()), unsigned(command->size()), command->value, 0,
                               HANTEK_ATTEMPTS);
    }

    /**
     * @return Returns the raw libusb device
     */
    inline libusb_device *getRawDevice() const { return device; }

    /**
     * @return Return the unique usb device id {@link USBDevice::computeUSBdeviceID()}.
     */
    inline UniqueUSBid getUniqueUSBDeviceID() const { return uniqueUSBdeviceID; }
    /**
     * ID built from bus, port, VID, PID and FW version
     */
    static UniqueUSBid computeUSBdeviceID( libusb_device *device );

    /// \brief Get the oscilloscope model.
    /// \return The ::Model of the connected Hantek DSO.
    inline const DSOModel *getModel() const { return model; }
    /**
     * Usually a maximum packet length for in and outgoing packets is determined
     * by the underlying implementation and usb specification. E.g. the roll buffer
     * mode uses the maximum in length for transfer. Some devices do not support
     * that much data though and need an artification restriction.
     */
    inline void overwriteInPacketLength(unsigned len) { inPacketLength = len; }

  protected:
//     int claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endpointOut, int endPointIn);
//     int claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endPointIn);
    int claimInterface( const libusb_interface_descriptor *interfaceDescriptor );

    // Device model data
    DSOModel* model;

    // Libusb specific variables
    struct libusb_device_descriptor descriptor;
    libusb_device *device; ///< The USB handle for the oscilloscope
    libusb_device_handle *handle = nullptr;
    unsigned findIteration;
    const UniqueUSBid uniqueUSBdeviceID;
    int interface;
    unsigned outPacketLength; ///< Packet length for the OUT endpoint
    unsigned inPacketLength;  ///< Packet length for the IN endpoint

  signals:
    void deviceDisconnected(); ///< The device has been disconnected
};
