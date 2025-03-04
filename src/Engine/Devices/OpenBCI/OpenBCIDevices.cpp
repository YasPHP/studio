/****************************************************************************
**
** Copyright 2019 neuromore co
** Contact: https://neuromore.com/contact
**
** Commercial License Usage
** Licensees holding valid commercial neuromore licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and neuromore. For licensing terms
** and conditions see https://neuromore.com/licensing. For further
** information use the contact form at https://neuromore.com/contact.
**
** neuromore Public License Usage
** Alternatively, this file may be used under the terms of the neuromore
** Public License version 1 as published by neuromore co with exceptions as 
** appearing in the file neuromore-class-exception.md included in the 
** packaging of this file. Please review the following information to 
** ensure the neuromore Public License requirements will be met: 
** https://neuromore.com/npl
**
****************************************************************************/

// include precompiled header
#include <Engine/Precompiled.h>

// include required files
#include "OpenBCIDevices.h"
#include "../../EngineManager.h"
#include "../../Core/LogManager.h"

#ifdef INCLUDE_DEVICE_OPENBCI

using namespace Core;

// base class commons
void OpenBCIDeviceBase::CreateSensors()
{
	BciDevice::CreateSensors();

	mAccForwardSensor = new Sensor("Acc (Forward)", 250);
	mAccForwardSensor->GetChannel()->SetMinValue(-2000.0);
	mAccForwardSensor->GetChannel()->SetMaxValue(1996.1);
	mAccForwardSensor->GetChannel()->SetUnit("mm/s^2");
	AddSensor(mAccForwardSensor);

	mAccUpSensor = new Sensor("Acc (Up)", 250);
	mAccUpSensor->GetChannel()->SetMinValue(-2000.0);
	mAccUpSensor->GetChannel()->SetMaxValue(1996.1);
	mAccUpSensor->GetChannel()->SetUnit("mm/s^2");
	AddSensor(mAccUpSensor);

	mAccLeftSensor = new Sensor("Acc (Left)", 250);
	mAccLeftSensor->GetChannel()->SetMinValue(-2000.0);
	mAccLeftSensor->GetChannel()->SetMaxValue(1996.1);
	mAccLeftSensor->GetChannel()->SetUnit("mm/s^2");
	AddSensor(mAccLeftSensor);
}


//
// OpenBCI without Daisy module
//

// constructor
OpenBCIDevice::OpenBCIDevice(DeviceDriver* driver) : OpenBCIDeviceBase(driver)
{
	LogDetailedInfo("Constructing OpenBCI headset ...");

	// create all sensors
	CreateSensors();
}


// destructor
OpenBCIDevice::~OpenBCIDevice()
{
	LogDetailedInfo("Destructing OpenBCI headset ...");
}


// get the available electrodes of the neuro headset
void OpenBCIDevice::CreateElectrodes()
{
	mElectrodes.Clear();
	mElectrodes.Reserve(8);

	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("Fp1"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("Fp2"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("C5"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("C6"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("P7"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("O1"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("O2"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("P8"));
}


//
// OpenBCI + Daisy module
//

// constructor
OpenBCIDaisyDevice::OpenBCIDaisyDevice(DeviceDriver* driver) : OpenBCIDeviceBase(driver)
{
	LogDetailedInfo("Constructing OpenBCI + Daisy device ...");

	// create all sensors
	CreateSensors();
}


// destructor
OpenBCIDaisyDevice::~OpenBCIDaisyDevice()
{
	LogDetailedInfo("Destructing OpenBCI + Daisy device...");
}


// default: ultracortex positions
void OpenBCIDaisyDevice::CreateElectrodes()
{
	mElectrodes.Clear();
	mElectrodes.Reserve(16);

	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("P3"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("P4"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("Oz"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("Fz"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("T5"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("T6"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("F3"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("F4"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("T3"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("T4"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("C3"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("C4"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("Cz"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("O1"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("O2"));
	mElectrodes.Add(GetEEGElectrodes()->GetElectrodeByID("Pz"));
}


#endif
