/****************************************************************************
**
** Copyright 2022 neuromore co
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

#ifndef __NEUROMORE_FOGCONTROLNODE_H
#define __NEUROMORE_FOGCONTROLNODE_H

// include the required headers
#include "../Config.h"
#include "../Core/StandardHeaders.h"
#include "CustomFeedbackNode.h"

class ENGINE_API FogControlNode : public CustomFeedbackNode
{
	public:
		enum { TYPE_ID				= 0x0066 };

        enum { INPUTPORT_VALUE		= 0 };

        enum EError
		{
			ERROR_VALUE_RANGE			= GraphObjectError::ERROR_RUNTIME		| 0x02,
		};

		static const char* Uuid () { return "b2c29626-b263-4f25-9675-8b65bfa2b629"; }

		FogControlNode(Graph* parentGraph);
		virtual ~FogControlNode();

        double GetCurrentValue(uint32 channelIndex = 0) const;
        bool IsEmpty(uint32 channelIndex = 0) const;

		// initialize & update
		void Init() override;
		void ReInit(const Core::Time& elapsed, const Core::Time& delta) override;
		void Reset() override;
		void Update(const Core::Time& elapsed, const Core::Time& delta) override;
		void OnAttributesChanged() override;

		Core::Color GetColor() const override                                   { return Core::RGBA(0,229,189); }
		uint32 GetType() const override											{ return TYPE_ID; }
		const char* GetTypeUuid() const override final							{ return Uuid(); }
		const char* GetReadableType() const override							{ return "Fog Control"; }
		const char* GetRuleName() const override final							{ return "NODE_FogFeedback"; }
		GraphObject* Clone(Graph* graph) override								{ FogControlNode* clone = new FogControlNode(graph); return clone; }

		void SetName(const char* name) override;

        double GetRangeMin() const override                                     { return 0.0; }
        double GetRangeMax() const override                                     { return 1.0; }

        void WriteOscMessage(OscPacketParser::OutStream* outStream) override;

    private:
        void HideAttributes();
};

#endif
