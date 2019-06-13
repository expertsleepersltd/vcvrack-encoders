#include "Encoders.hpp"
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

	ModuleSMUX()
	: m_flipL( 0 )
	, m_flipR( 0 )
	, m_parity( 0 )
	{
		config( kNumParams, 4, 2, kNumLights );

		configParam(ModuleSMUX::kParamFlipL, 0.0f, 1.0f, 0.0f, "");
		configParam(ModuleSMUX::kParamFlipR, 0.0f, 1.0f, 0.0f, "");

		memset( m_values, 0, sizeof m_values );
	}
	void process(const ProcessArgs &args) override;
	
	dsp::SchmittTrigger	m_flipLTrigger, m_flipRTrigger;
	int		m_flipL, m_flipR;
	
	int		m_parity;
	float	m_values[4];

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();
		
		json_object_set_new(rootJ, "m_flipL", json_boolean(m_flipL));
		json_object_set_new(rootJ, "m_flipR", json_boolean(m_flipR));

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override
	{
		json_t *m_flipLJ = json_object_get(rootJ, "m_flipL");
		if (m_flipLJ)
			m_flipL = json_is_true(m_flipLJ);
		json_t *m_flipRJ = json_object_get(rootJ, "m_flipR");
		if (m_flipRJ)
			m_flipR = json_is_true(m_flipRJ);
	}
};

void ModuleSMUX::process(const ProcessArgs &args)
{
	if ( m_flipLTrigger.process( params[kParamFlipL].getValue() ) )
		m_flipL = 1 - m_flipL;
	if ( m_flipRTrigger.process( params[kParamFlipR].getValue() ) )
		m_flipR = 1 - m_flipR;
	lights[kLightFlipL].value = m_flipL;
	lights[kLightFlipR].value = m_flipR;
	
	if ( !m_parity )
	{
		m_values[0] = inputs[  m_flipL].getVoltage();
		m_values[1] = inputs[1-m_flipL].getVoltage();
		m_values[2] = inputs[2+m_flipR].getVoltage();
		m_values[3] = inputs[3-m_flipR].getVoltage();
	}

	outputs[0].setVoltage(m_values[  m_parity]);
	outputs[1].setVoltage(m_values[2+m_parity]);

	m_parity = 1 - m_parity;
}

struct ModuleSMUXWidget : ModuleWidget
{
	ModuleSMUXWidget(ModuleSMUX *module)
	{
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SMUX.svg")));

		int buttonX = 14;
		addParam(createParam<LEDButton>(Vec(buttonX, 200), module, ModuleSMUX::kParamFlipL));
		addChild(createLight<MediumLight<GreenLight>>(Vec(buttonX+4.4f, 200 + 4.4f), module, ModuleSMUX::kLightFlipL));

		addParam(createParam<LEDButton>(Vec(buttonX, 200+50), module, ModuleSMUX::kParamFlipR));
		addChild(createLight<MediumLight<GreenLight>>(Vec(buttonX+4.4f, 200+50 + 4.4f), module, ModuleSMUX::kLightFlipR));

		for ( int i=0; i<4; ++i )
			addInput(createInput<PJ301MPort>(Vec(17, 45+33*i), module, i));

		for ( int i=0; i<2; ++i )
			addOutput(createOutput<PJ301MPort>(Vec(17, (9+i)*33), module, i));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelSMUX = createModel<ModuleSMUX, ModuleSMUXWidget>( "ExpertSleepers-Encoders-SMUX" );
