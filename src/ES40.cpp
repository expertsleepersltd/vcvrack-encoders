#include "Encoders.hpp"

struct ModuleES40 : Module
{
	ModuleES40()
	{
		config( 0, 5, 2, 0 );
	}
	void process(const ProcessArgs &args) override;
};

#define btf_n_factor (-(float)0x800000)
#define btf_p_factor ((float)0x800000)

#define ES_BITSTOFLOAT( bits )	\
( ( bits & 0x800000 ) ? ( (((float)(0xffffff&(-(int)(bits)))) / btf_n_factor ) ) : ( ((float)(bits)) / btf_p_factor ) )

void ModuleES40::process(const ProcessArgs &args)
{
	int b[5];

	for ( int i=0; i<5; ++i )
	{
		float f = inputs[i].getVoltage();
		int bi = (int)f;
		b[i] = std::max( 0, std::min( 255, bi ) );
	}
	
    int bitsL = ( b[0] << 16 ) | ( b[1] << 8 ) | ( ( b[4] << 0 ) & 0xf0 );
	int bitsR = ( b[2] << 16 ) | ( b[3] << 8 ) | ( ( b[4] << 4 ) & 0xf0 );

	outputs[0].setVoltage(10.0f * ES_BITSTOFLOAT( bitsL ));
	outputs[1].setVoltage(10.0f * ES_BITSTOFLOAT( bitsR ));
}

struct ModuleES40Widget : ModuleWidget
{
	ModuleES40Widget(ModuleES40 *module)
	{
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ES40.svg")));

		for ( int i=0; i<5; ++i )
			addInput(createInput<PJ301MPort>(Vec(17, 45+33*i), module, i));

		for ( int i=0; i<2; ++i )
			addOutput(createOutput<PJ301MPort>(Vec(17, (9+i)*33), module, i));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelES40 = createModel<ModuleES40, ModuleES40Widget>( "ExpertSleepers-Encoders-ES40" );
