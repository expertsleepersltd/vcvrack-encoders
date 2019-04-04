#include "Encoders.hpp"
#include "dsp/digital.hpp"
#include <string.h>

struct ModuleSMUX : Module
{
	enum {
		kParamFlipL = 0,
		kParamFlipR,
		
		kNumParams
	};
	
	enum {
		kLightFlipL = 0,
		kLightFlipR,
		
		kNumLights
	};

	ModuleSMUX() : Module( kNumParams, 4, 2, kNumLights )
	, m_flipL( 0 )
	, m_flipR( 0 )
	, m_parity( 0 )
	{
		memset( m_values, 0, sizeof m_values );
	}
	void step() override;
	
	SchmittTrigger	m_flipLTrigger, m_flipRTrigger;
	int		m_flipL, m_flipR;
	
	int		m_parity;
	float	m_values[4];

	json_t *toJson() override
	{
		json_t *rootJ = json_object();
		
		json_object_set_new(rootJ, "m_flipL", json_boolean(m_flipL));
		json_object_set_new(rootJ, "m_flipR", json_boolean(m_flipR));

		return rootJ;
	}
	
	void fromJson(json_t *rootJ) override
	{
		json_t *m_flipLJ = json_object_get(rootJ, "m_flipL");
		if (m_flipLJ)
			m_flipL = json_is_true(m_flipLJ);
		json_t *m_flipRJ = json_object_get(rootJ, "m_flipR");
		if (m_flipRJ)
			m_flipR = json_is_true(m_flipRJ);
	}
};

void ModuleSMUX::step()
{
	if ( m_flipLTrigger.process( params[kParamFlipL].value ) )
		m_flipL = 1 - m_flipL;
	if ( m_flipRTrigger.process( params[kParamFlipR].value ) )
		m_flipR = 1 - m_flipR;
	lights[kLightFlipL].value = m_flipL;
	lights[kLightFlipR].value = m_flipR;
	
	if ( !m_parity )
	{
		m_values[0] = inputs[  m_flipL].value;
		m_values[1] = inputs[1-m_flipL].value;
		m_values[2] = inputs[2+m_flipR].value;
		m_values[3] = inputs[3-m_flipR].value;
	}

	outputs[0].value = m_values[  m_parity];
	outputs[1].value = m_values[2+m_parity];

	m_parity = 1 - m_parity;
}

struct ModuleSMUXWidget : ModuleWidget
{
	ModuleSMUXWidget(ModuleSMUX *module) : ModuleWidget(module)
	{
		setPanel(SVG::load(assetPlugin(plugin, "res/SMUX.svg")));

		int buttonX = 14;
		addParam(ParamWidget::create<LEDButton>(Vec(buttonX, 200), module, ModuleSMUX::kParamFlipL, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(buttonX+4.4f, 200 + 4.4f), module, ModuleSMUX::kLightFlipL));

		addParam(ParamWidget::create<LEDButton>(Vec(buttonX, 200+50), module, ModuleSMUX::kParamFlipR, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(buttonX+4.4f, 200+50 + 4.4f), module, ModuleSMUX::kLightFlipR));

		for ( int i=0; i<4; ++i )
			addInput(Port::create<PJ301MPort>(Vec(17, 45+33*i), Port::INPUT, module, i));

		for ( int i=0; i<2; ++i )
			addOutput(Port::create<PJ301MPort>(Vec(17, (9+i)*33), Port::OUTPUT, module, i));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelSMUX = Model::create<ModuleSMUX, ModuleSMUXWidget>( "Expert Sleepers", "ExpertSleepers-Encoders-SMUX", "SMUX", EXTERNAL_TAG );
