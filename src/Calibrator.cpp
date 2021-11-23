#include "Encoders.hpp"
#include <string.h>
#include <algorithm>
#include <array>

#define kNumSamples (20000)

struct ModuleCalibrator : Module
{
	enum {
		kParamCalibrateInput = 0,
		kParamCalibrateOutput,
		
		kNumParams
	};
	
	enum {
		kLightCalibratingInput = 0,
		kLightCalibratingOutput,
		
		kNumLights
	};
	
	enum {
		kStateIdle = 0,
		kStateInputWait1,
		kStateInputSample1,
		kStateInputWait2,
		kStateInputSample2,
		kStateOutputWait1,
		kStateOutputSample1,
		kStateOutputWait2,
		kStateOutputSample2,
	};

	ModuleCalibrator()
	: status( "Idle" )
	, in0V( 0.0f )
	, in3V( 3.0f )
	, out0V( 0.0f )
	, out3V( 3.0f )
	, state( kStateIdle )
	, flashInputButton( false )
	, flashOutputButton( false )
	, flashCounter( 0.0f )
	{
		config( kNumParams, 2, 2, kNumLights );

		configParam(ModuleCalibrator::kParamCalibrateInput, 0.0f, 1.0f, 0.0f, "");
		configParam(ModuleCalibrator::kParamCalibrateOutput, 0.0f, 1.0f, 0.0f, "");
	}
	void onReset() override
	{
		in0V = 0.0f;
		in3V = 3.0f;
		out0V = 0.0f;
		out3V = 3.0f;
	}
	void process(const ProcessArgs &args) override;

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();

		json_object_set_new(rootJ, "in0V", json_real(in0V));
		json_object_set_new(rootJ, "in3V", json_real(in3V));
		json_object_set_new(rootJ, "out0V", json_real(out0V));
		json_object_set_new(rootJ, "out3V", json_real(out3V));

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override
	{
		json_t *j;
		j = json_object_get(rootJ, "in0V");
		if (j)
			in0V = json_real_value(j);
		j = json_object_get(rootJ, "in3V");
		if (j)
			in3V = json_real_value(j);
		j = json_object_get(rootJ, "out0V");
		if (j)
			out0V = json_real_value(j);
		j = json_object_get(rootJ, "out3V");
		if (j)
			out3V = json_real_value(j);
	}
	
	float findMedianSample(void);

	dsp::SchmittTrigger	m_calibrateInputTrigger;
	dsp::SchmittTrigger	m_calibrateOutputTrigger;
	
	std::string status;
	
	float	in0V, in3V;
	float	out0V, out3V;
	int 	state;
	int		count;
	
	bool	flashInputButton;
	bool	flashOutputButton;
	float 	flashCounter;
	
	std::array<float, kNumSamples>	samples;
};

void ModuleCalibrator::process(const ProcessArgs &args)
{
	if ( flashInputButton )
	{
		flashCounter += 10.0f/args.sampleRate;
		if ( flashCounter > 1.0f )
		{
			flashCounter -= 1.0f;
			lights[kLightCalibratingInput].value = !lights[kLightCalibratingInput].value;
		}
	}
	else
		lights[kLightCalibratingInput].value = params[kParamCalibrateInput].getValue();
	
	if ( flashOutputButton )
	{
		flashCounter += 10.0f/args.sampleRate;
		if ( flashCounter > 1.0f )
		{
			flashCounter -= 1.0f;
			lights[kLightCalibratingOutput].value = !lights[kLightCalibratingOutput].value;
		}
	}
	else
		lights[kLightCalibratingOutput].value = params[kParamCalibrateOutput].getValue();

	float in = inputs[0].getVoltage();
	float v = 3.0f * ( in - in0V )/( in3V - in0V );
	outputs[0].setVoltage( v );
	
	in = inputs[1].getVoltage();
	v = ( in * ( in3V - in0V ) - 3.0f * ( out0V - in0V ) )/( out3V - out0V );
	outputs[1].setVoltage( v );
	
	switch ( state )
	{
		case kStateInputWait1:
		    if ( m_calibrateInputTrigger.process( params[kParamCalibrateInput].getValue() ) )
		    {
		    	state = kStateInputSample1;
		    	status = "Sampling with no input";
		    	count = 0;
		    	flashInputButton = false;
			}
			return;
		case kStateInputSample1:
			samples[ count++ ] = inputs[0].getVoltage();
			if ( count >= kNumSamples )
			{
				in0V = findMedianSample();
				state = kStateInputWait2;
				status = "Connect 3V reference and confirm";
				flashInputButton = true;
			}
			return;
		case kStateInputWait2:
		    if ( m_calibrateInputTrigger.process( params[kParamCalibrateInput].getValue() ) )
		    {
		    	state = kStateInputSample2;
		    	status = "Sampling with 3V input";
		    	count = 0;
		    	flashInputButton = false;
			}
			return;
		case kStateInputSample2:
			samples[ count++ ] = inputs[0].getVoltage();
			if ( count >= kNumSamples )
			{
				in3V = findMedianSample();
				state = kStateIdle;
				status = "Idle";
			}
			return;
		case kStateOutputWait1:
			outputs[1].setVoltage( 0.0f );
		    if ( m_calibrateOutputTrigger.process( params[kParamCalibrateOutput].getValue() ) )
		    {
		    	state = kStateOutputSample1;
		    	status = "Sampling at 0V";
		    	count = 0;
		    	flashOutputButton = false;
			}
			return;
		case kStateOutputSample1:
			outputs[1].setVoltage( 0.0f );
			samples[ count++ ] = inputs[0].getVoltage();
			if ( count >= kNumSamples )
			{
				out0V = findMedianSample();
				state = kStateOutputWait2;
				status = "Settling ...";
				count = 0;
			}
			return;
		case kStateOutputWait2:
			outputs[1].setVoltage( 3.0f );
			if ( ++count > args.sampleRate/2 )
		    {
		    	state = kStateOutputSample2;
		    	status = "Sampling at 3V";
		    	count = 0;
			}
			return;
		case kStateOutputSample2:
			outputs[1].setVoltage( 3.0f );
			samples[ count++ ] = inputs[0].getVoltage();
			if ( count >= kNumSamples )
			{
				out3V = findMedianSample();
				state = kStateIdle;
				status = "Idle";
			}
			return;
	}

    if ( m_calibrateInputTrigger.process( params[kParamCalibrateInput].getValue() ) )
    {
    	state = kStateInputWait1;
    	status = "Ensure input jack is dis- connected and confirm";
    	flashInputButton = true;
    	return;
    }
    if ( m_calibrateOutputTrigger.process( params[kParamCalibrateOutput].getValue() ) )
    {
    	state = kStateOutputWait1;
    	status = "Connect output jack to input and confirm";
    	flashOutputButton = true;
    	return;
    }
}

float ModuleCalibrator::findMedianSample(void)
{
	std::sort( samples.begin(), samples.end() );
	return samples[ kNumSamples/2 ];
}

struct StatusWindowTextField : LedDisplayTextField
{
	const std::string* source;
	
	void onButton(const event::Button& e) override
	{
		OpaqueWidget::onButton(e);
	}
	void draw(const DrawArgs& args) override
	{
		if ( source )
			setText( *source );
		LedDisplayTextField::draw( args );
	}
};

struct StatusWindow : LedDisplay {
	void setModule(ModuleCalibrator* module) {
		StatusWindowTextField* textField = createWidget<StatusWindowTextField>(Vec(0, 0));
		textField->box.size = box.size;
		textField->multiline = true;
		textField->source = module ? &module->status : NULL;
		addChild(textField);
	}
};

struct ModuleCalibratorWidget : ModuleWidget
{
    StatusWindow* textField;

	ModuleCalibratorWidget(ModuleCalibrator *module)
	{
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Calibrator.svg")));
		
		int socketX = 5;

		addInput(createInput<PJ301MPort>(Vec(socketX, 230), module, 0));
		addOutput(createOutput<PJ301MPort>(Vec(socketX, 260), module, 0));

		addInput(createInput<PJ301MPort>(Vec(socketX, 300), module, 1));
		addOutput(createOutput<PJ301MPort>(Vec(socketX, 330), module, 1));

		int buttonX = 8;
		
		addParam(createParam<LEDButton>(Vec(buttonX, 160), module, ModuleCalibrator::kParamCalibrateInput));
		addChild(createLight<MediumLight<RedLight>>(Vec(buttonX+4.4f, 160 + 4.4f), module, ModuleCalibrator::kLightCalibratingInput));

		addParam(createParam<LEDButton>(Vec(buttonX, 190), module, ModuleCalibrator::kParamCalibrateOutput));
		addChild(createLight<MediumLight<RedLight>>(Vec(buttonX+4.4f, 190 + 4.4f), module, ModuleCalibrator::kLightCalibratingOutput));

		textField = createWidget<StatusWindow>( Vec(5, 45) );
		textField->box.size = Vec(80, 100);
		textField->setModule( module );
		addChild(textField);
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelCalibrator = createModel<ModuleCalibrator, ModuleCalibratorWidget>( "ExpertSleepers-Encoders-Calibrator" );
