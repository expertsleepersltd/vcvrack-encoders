#include "Encoders.hpp"

Plugin *pluginInstance;

void init(Plugin *p)
{
	pluginInstance = p;

	p->addModel( model8GT );
	p->addModel( model8CV );
	p->addModel( modelES40 );
	p->addModel( modelES5 );
	p->addModel( modelSMUX );
	p->addModel( modelCalibrator );
}
