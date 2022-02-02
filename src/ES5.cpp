#include "Encoders.hpp"

struct ModuleES5 : Module
{
	ModuleES5()
	: category( 0 )
	{
		config( 0, 6, 2, 0 );
	}
	void process(const ProcessArgs &args) override;
	
	int category;

	json_t *dataToJson() override
	{
		json_t *rootJ = json_object();
		
		json_object_set_new(rootJ, "category", json_integer(category));
		
		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override
	{
		json_t *categoryJ = json_object_get(rootJ, "category");
		if (categoryJ)
			category = json_integer_value(categoryJ);
	}
};

#define ES_BITSTOFLOAT( bits )	\
( ( bits & 0x800000 ) ? ( (((float)(0xffffff&(-(int)(bits)))) / btf_n_factor ) ) : ( ((float)(bits)) / btf_p_factor ) )

void ModuleES5::process(const ProcessArgs &args)
{
	int b[6];

	for ( int i=0; i<6; ++i )
	{
		float f = inputs[i].getVoltage();
		int bi = (int)f;
		b[i] = std::max( 0, std::min( 255, bi ) );
	}
	
	int bitsL = ( b[0] << 16 ) | ( b[1] << 8 ) | b[2];
	int bitsR = ( b[3] << 16 ) | ( b[4] << 8 ) | b[5];

	float btf_n_factor, btf_p_factor;
	if ( category == 2 ) {
		{
			btf_n_factor = -(float)(0x7fffff);			btf_p_factor = (float)(0x7fffff);
		}
	}
	else if ( category != 0 ) {
		{
			btf_n_factor = -(float)(0x7fff80);			btf_p_factor = (float)(0x7fff80);
		}
	}
	else {
		{
			btf_n_factor = -(float)0x800000;			btf_p_factor = (float)0x800000;
		}
	}

	outputs[0].setVoltage(10.0f * ES_BITSTOFLOAT( bitsL ));
	outputs[1].setVoltage(10.0f * ES_BITSTOFLOAT( bitsR ));
}

struct ModuleES5Widget : ModuleWidget
{
	ModuleES5Widget(ModuleES5 *module)
	{
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ES5.svg")));

		for ( int i=0; i<6; ++i )
			addInput(createInput<PJ301MPort>(Vec(17, 45+33*i), module, i));

		for ( int i=0; i<2; ++i )
			addOutput(createOutput<PJ301MPort>(Vec(17, (9+i)*33), module, i));
	}

	void appendContextMenu(Menu *menu) override
	{
		ModuleES5 *module = dynamic_cast<ModuleES5*>(this->module);
		
		struct CategoryItem : MenuItem
		{
			ModuleES5 *module;
			int category;
			virtual void onAction(const event::Action &e) override
			{
				module->category = category;
			}
		};
		
		struct CategoriesItem : MenuItem
		{
			ModuleES5 *module;
			Menu *createChildMenu() override
			{
				Menu *menu = new Menu();
				char const * const names[] = { "A", "B", "C" };
				for (int i = 0; i < 3; i++) {
					CategoryItem *item = createMenuItem<CategoryItem>(names[i], CHECKMARK(module->category == i));
					item->module = module;
					item->category = i;
					menu->addChild(item);
				}
				return menu;
			}
		};
		
		menu->addChild(construct<MenuLabel>());
		CategoriesItem *item = createMenuItem<CategoriesItem>( "Category" );
		item->module = module;
		menu->addChild(item);
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelES5 = createModel<ModuleES5, ModuleES5Widget>( "ExpertSleepers-Encoders-ES5" );
