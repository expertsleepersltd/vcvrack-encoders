#include "Encoders.hpp"

struct Module8CV : Module
{
	Module8CV() : m_phase( 0 )
				, m_value( 0 )
	{
		config( 0, 8, 1, 0 );
	}
	void process(const ProcessArgs &args) override;
	
	int 			m_phase;
	unsigned int	m_value;
};

void Module8CV::process(const ProcessArgs &args)
{
	int state = ( m_phase >> 1 ) & 3;
	int dac = ( m_phase >> 3 ) & 7;
	
	if ( state == 0 )
	{
		{
			// take the next dac in sequence
			float s = inputs[ dac ].getVoltage() * 384;
			m_value = 2048 + (int)( std::max( -2048.0f, std::min( 2047.0f, s ) ) );
		}
	}

	bool smuxProof = false;
	if ( !smuxProof )
		m_phase = m_phase & ~1;
	int phaseInc = smuxProof ? 1 : 2;

	m_phase += phaseInc;
	if ( ( m_phase & 7 ) == 6 )
		m_phase += 2;
	m_phase = m_phase & 63;
	
	unsigned int out = ( state == 0 ) ? ( 0x80 | ( m_value & 0x1f ) )
				   : ( ( state == 1 ) ? ( ( m_value >> 5 ) & 0x1f )
	   			   : ( ( ( dac > 3 ) ? 0x40 : 0x20 ) | ( m_value >> 10 ) | ( ( dac & 3 ) << 2 ) ) );

	outputs[0].setVoltage(out);
}

struct Module8CVWidget : ModuleWidget
{
	Module8CVWidget(Module8CV *module)
	{
		setModule(module);
		
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/8CV.svg")));

		for ( int i=0; i<8; ++i )
			addInput(createInput<PJ301MPort>(Vec(17, 45+33*i), module, i));

		addOutput(createOutput<PJ301MPort>(Vec(17, 10*33), module, 0));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *model8CV = createModel<Module8CV, Module8CVWidget>( "ExpertSleepers-Encoders-8CV" );
