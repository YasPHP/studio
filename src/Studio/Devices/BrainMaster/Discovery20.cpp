
// include precompiled header
#include <Studio/Precompiled.h>

#ifdef INCLUDE_DEVICE_BRAINMASTER

// WINDOWS X86 ONLY
#if defined(_WIN32) && (defined(_M_IX86) || defined(_X86_) || defined(__i386__) || defined(__i686__))

#include "Discovery20.h"

// Windows API DEFINE
#ifndef INITGUID
#define INITGUID
#endif

// Windows API
#include <Ntddser.h>
#include <SetupAPI.h> // SetupAPI
#include <cfgmgr32.h> // for MAX_DEVICE_ID_LEN and CM_Get_Device_ID
#include <devpkey.h>  // for DEVPKEY_Device_FriendlyName

int Discovery20::findCOM()
{
   HDEVINFO        devinfo;
   SP_DEVINFO_DATA devdata;
   CONFIGRET       status;
   TCHAR           devid[MAX_DEVICE_ID_LEN];
   DEVPROPTYPE     proptype;
   DWORD           size;
   WCHAR           buffer[4096];

   // device friendly name prefix to scan for
   static constexpr WCHAR scan[] = 
      L"Discovery 24E Module (COM";

   // init devdata
   memset(&devdata, 0, sizeof(devdata));
   devdata.cbSize = sizeof(devdata);

   // query comports devinfo
   devinfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (devinfo == INVALID_HANDLE_VALUE)
      return 0;

   // iterate them
   for (DWORD i = 0; ; i++)
   {
      // query devdata
      if (!SetupDiEnumDeviceInfo(devinfo, i, &devdata))
         break;

      // query devid
      status = CM_Get_Device_ID(devdata.DevInst, devid, MAX_DEVICE_ID_LEN, 0);
      if (status != CR_SUCCESS)
         continue;

      // query devprop
      if (!SetupDiGetDevicePropertyW(devinfo, &devdata, &DEVPKEY_Device_FriendlyName, &proptype, (BYTE*)buffer, sizeof(buffer), &size, 0))
         continue;

      // compare and extract com port num
      const int scanlen = lstrlenW(scan);
      if (wcsncmp(buffer, scan, scanlen) == 0)
      {
         wchar_t* start = &buffer[scanlen];
         wchar_t* end   = &buffer[size-2];
         long x = wcstol(start, &end, 10);
         if (((x != 0) & (x != LONG_MIN) & (x != LONG_MAX)) != 0)
            return x;
      }
   }

   return 0;
}

void Discovery20::processQueue()
{
   switch (mState)
   {
   case State::UNSYNCED:
   {
      constexpr size_t BUFSIZE = Frame::SIZE*2;
      if (mSDK.AtlReadData(mBuffer, BUFSIZE) != BUFSIZE) {
         disconnect();
         return;
      }
      for (uint32_t i = 0; i < Frame::SIZE; i++)
      {
         const uint8_t c1 = mBuffer[i];
         const uint8_t c2 = mBuffer[i+Frame::SIZE];
         if (isSyncPair(c1, c2))
         {
            if (i > 0) {
               mNumBytes = BUFSIZE - (i+Frame::SIZE);
               std::memcpy(mFrame.data, &mBuffer[i+Frame::SIZE], mNumBytes);
            }
            else {
               mNumBytes = 0;
            }
            mState = State::SYNCING;
            mNumSyncs = 0;
            mNextSync = c2;
            mCallback.onSyncStart(*this, c1, c2);
            return;
         }
      }
      break;
   }
   case State::SYNCING:
   {
      if (mFrame.sync != mNextSync) {
         mState = State::UNSYNCED;
         mNumSyncs = 0;
         mCallback.onSyncFail(*this, mNextSync, mFrame.sync);
         break;
      }
      if (mNumSyncs >= MINSYNCS) {
         mState = State::SYNCED;
         stepSync();
         mCallback.onSyncSuccess(*this);
         break;
      }
      if (mSDK.AtlReadData(&mFrame.data[mNumBytes], Frame::SIZE-mNumBytes) != Frame::SIZE-mNumBytes) {
         disconnect();
         return;
      }
      if (mNumBytes == 0)
         stepSync();
      mNumBytes = 0;
      break;
   }
   case State::SYNCED:
   {
      if (mSDK.AtlReadData(mFrame.data, Frame::SIZE) != Frame::SIZE) {
         disconnect();
         return;
      }
      if (mFrame.sync != mNextSync) {
         mState = State::UNSYNCED;
         mNumSyncs = 0;
         mCallback.onSyncLost(*this);
         break;
      }
      mFrame.extract(mChannels, mImpedancesRef, mImpedancesAct);
      mCallback.onFrame(*this, mFrame, mChannels);
      stepSync();
      break;
   }
   default: break;
   }
}

bool Discovery20::find()
{
   if (mState != State::DISCONNECTED)
      return false;

   // try find the device com port
   mPort = findCOM();

   // raise callback
   if (mPort)
      mCallback.onDeviceFound(*this, mPort);

   // found or not
   return mPort != 0;
}

Discovery20::ConnectResult Discovery20::connect(const char* codekey, const char* serialnumber, const char* passkey)
{
   if (!mSDK.Handle)
      return ConnectResult::SDK_NOT_LOADED;

   // must be disconnected to connect
   if (mState != State::DISCONNECTED)
      return ConnectResult::STATE_NOT_DISCONNECTED;

   // no device discovered or found
   if (!mPort)
      return ConnectResult::NO_DEVICE_DISCOVERED;

   // connect at 9600 baud first
   if (!mSDK.AtlOpenPort(mPort, 9600, &mHandle))
      return ConnectResult::OPEN_PORT_FAILED;

   // set to 460800 baud
   if (!mSDK.AtlSetBaudRate(SDK::BR460800)) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::SET_BAUD_RATE_FAILED;
   }

   // close port
   if (!mSDK.AtlClosePort(mPort))
      return ConnectResult::CLOSE_PORT_FAILED;

   // now open at 460800 baud
   if (!mSDK.AtlOpenPort(mPort, 460800, &mHandle))
      return ConnectResult::OPEN_PORT_FAILED;

   // setup serial port buffers
   if (!::SetupComm(mHandle, BUFFERSIZE, BUFFERSIZE)) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::BUFFER_INCREASE_FAILED;
   }

   // login to the device (and validate fw version response)
   const int32_t LOGINRETURN = mSDK.BmrLoginDevice(
      (char*)codekey, (char*)serialnumber, (char*)passkey);

   // validate successful login
   if (!LOGINRETURN) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::CREDENTIALS_WRONG;
   }

   // validate fw
   if (LOGINRETURN != SDK::LoginCodes::WIDEB2E) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::UNSUPPORTED_FIRMWARE;
   }

   // validate impedance support
   if ((mSDK.AtlPeek(0xb605) & 1) == 0) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::IMPEDANCE_NOT_SUPPORTED;
   }

   // set to expected sampling rate
   if (!mSDK.AtlWriteSamplingRate(SAMPLERATE)) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::SET_SAMPLERATE_FAILED;
   }

   // and validate it
   if (mSDK.AtlReadSamplingRate() != SAMPLERATE) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::READ_SAMPLERATE_FAILED;
   }

   // clear any specials that might be set
   if (!mSDK.AtlClearSpecials()) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::CLEAR_SPECIALS_FAILED;
   }

   // enable specialdata in frame (78 instead of 75 bytes)
   if (!mSDK.AtlSelectSpecial(0xFF)) {
      mSDK.AtlClosePort(mPort);
      return ConnectResult::ENABLE_IMPEDANCE_FAILED;
   }

   // flush
   mSDK.AtlFlush();

   // set state and raise callback
   mState = State::CONNECTED;
   mCallback.onDeviceConnected(*this);

   // success
   return ConnectResult::SUCCESS;
}

bool Discovery20::start()
{
   if (!mSDK.Handle)
      return false;

   // must be in connected state to init data streaming
   if (mState != State::CONNECTED)
      return false;

   // try to start streaming
   if (!mSDK.DiscStartModule())
      return false;

   // send 'Z' to enable impedance values in special data
   DWORD nw = 0;
   BOOL status = ::WriteFile(mHandle, "Z", 1, &nw, NULL);
   ::FlushFileBuffers(mHandle);

   // reset old buffers and values
   ::memset(&mBuffer, 0, sizeof(mBuffer));
   ::memset(&mChannels, 0, sizeof(mChannels));
   ::memset(&mImpedancesRef, 0, sizeof(mImpedancesRef));
   ::memset(&mImpedancesAct, 0, sizeof(mImpedancesAct));

   // set to unsynced state
   mState = State::UNSYNCED;

   // set streaming init tick
   mTickLastData = gettick();

   // success
   return true;
}

bool Discovery20::stop()
{
   // must be in streaming mode
   if (!isStreaming())
       return true;

   // flush
   mSDK.AtlFlush();

   // try to stop streaming
   if (!mSDK.DiscStopModule())
   {
      disconnect();
      return false;
   }

   // set to connected state
   mState = State::CONNECTED;

   // success
   return true;
}

bool Discovery20::disconnect()
{
   // already disconnected
   if (mState == State::DISCONNECTED)
      return true;

   // reset to 9600 baud and close port
   const BOOL R1 = mSDK.AtlSetBaudRate(SDK::BR9600);
   const BOOL R2 = mSDK.AtlClosePort(mPort);

   // set to disconnected and raise callback
   mState = State::DISCONNECTED;
   mCallback.onDeviceDisconnected(*this);

   // success
   return R1 && R2;
}

void Discovery20::update()
{
   // must be in streaming mode
   if (!isStreaming())
      return;

   // get amount of bytes and full frames in queue
   const DWORD NBYTES  = mSDK.AtlGetBytesInQue();
   const DWORD NFRAMES = NBYTES / Frame::SIZE;

   // current tick and delta since last frame
   const uint64_t TICK = gettick();
   const uint64_t DT  = TICK - mTickLastData;

   // how many frames must be available to start processing
   const DWORD WAITFRAMES = (mState == State::UNSYNCED) ? 2U : 1U;

   // got at least one or two frame(s)
   if (NFRAMES >= WAITFRAMES)
   {
      // update last frame tick
      mTickLastData = TICK;

      // process all frames in the queue
      for (DWORD i = 0; i < NFRAMES; i++)
         processQueue();
   }

   // check for timeout
   else if (DT > (TIMEOUT * 1000 * 1000))
   {
      mCallback.onDeviceTimeout(*this);
      disconnect();
   }
}
#endif
#endif
