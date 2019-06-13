#include "Encoders.hpp"
#include <string.h>

struct Module8GT : Module
{
	Module8GT()
	{
		config( 0, 8, 1, 0 );

		memset( m_bits, 0, sizeof m_bits );
	}
	void process(const ProcessArgs &args) override;
	
	bool	m_bits[8];
};

void Module8GT::process(const ProcessArgs &args)
{
	float out = 0.0f;
	
	for ( int i=0; i<8; ++i )
	{
		float f = inputs[i].getVoltage();
		if ( m_bits[i] )
		{
			if ( f < 0.5f )
				m_bits[i] = false;
		}
		else
		{
			if ( f >= 1.0f )
				m_bits[i] = true;
		}
		f = m_bits[i] ? (1<<i) : 0;
		out += f;
	}

	outputs[0].setVoltage(out);
}

struct Module8GTWidget : ModuleWidget
{
	Module8GTWidget(Module8GT *module)
	{
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/8GT.svg")));

		for ( int i=0; i<8; ++i )
			addInput(createInput<PJ301MPort>(Vec(17, 45+33*i), module, i));

		addOutput(createOutput<PJ301MPort>(Vec(17, 10*33), module, 0));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *model8GT = createModel<Module8GT, Module8GTWidget>( "ExpertSleepers-Encoders-8GT" );
