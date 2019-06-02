//----------------------------------------------------------------------------
//	Count Modula - Minimus Maximus Module
//	A Min/Max/Mean processor with gate outputs to indicate which inputs are at
//	the minimum or maximum level
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Polarizer.hpp"

struct MinimusMaximus : Module {
	enum ParamIds {
		BIAS_PARAM,
		BIAS_ON_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_MAX_OUTPUT,
		B_MAX_OUTPUT,
		C_MAX_OUTPUT,
		D_MAX_OUTPUT,
		A_MIN_OUTPUT,
		B_MIN_OUTPUT,
		C_MIN_OUTPUT,
		D_MIN_OUTPUT,
		MIN_OUTPUT,
		MAX_OUTPUT,
		AVG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	MinimusMaximus() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void MinimusMaximus::step() {
	
	float tot = 0.0f;
	int count = 0;
	float min = 99999.0f;
	float max = -99999.0f;
	float f = 0.0f;
	
	// calculate max/min amounts and requisites for average
	for (int i = 0; i < 4; i++) {
		bool useBias = (i == 3 && params[BIAS_ON_PARAM].value > 0.5f);
		
		if (inputs[A_INPUT+i].active || useBias) {
			// grab the value taking the bias setting of the last input into consideration
			f = useBias ? inputs[A_INPUT+i].normalize(params[BIAS_PARAM].value) : inputs[A_INPUT+i].value;
			
			count++;
			tot += f;
			
			if (f > max)
				max = f;
			
			if (f < min)
				min = f;
		}
	}
	
	// determine the comparator output levels
	float offset = (params[MODE_PARAM].value > 0.5f ? 5.0f : 0.0f);
		
	// cater for possibility of no inputs
	if (count > 0)
		outputs[AVG_OUTPUT].value = tot /(float)(count);
	else {
		// no inputs - set outputs to zero
		outputs[AVG_OUTPUT].value = 0.0f;
		max = min = 0.0f;
	}
	
	// set the min/max outputs
	outputs[MIN_OUTPUT].value = min;
	outputs[MAX_OUTPUT].value = max;

	// set the logic outputs
	for (int i = 0; i < 4; i++) {
		outputs[A_MIN_OUTPUT + i].value = boolToGate(inputs[A_INPUT+i].active && (inputs[A_INPUT+i].value == min)) - offset;
		outputs[A_MAX_OUTPUT + i].value = boolToGate(inputs[A_INPUT+i].active && (inputs[A_INPUT+i].value == max)) - offset;
	}
}

struct MinimusMaximusWidget : ModuleWidget {
MinimusMaximusWidget(MinimusMaximus *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MinimusMaximus.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// controls
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, MinimusMaximus::BIAS_ON_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, MinimusMaximus::BIAS_PARAM, -5.0f, 5.0f, 0.0f));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, MinimusMaximus::MODE_PARAM, 0.0f, 1.0f, 0.0f));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, MinimusMaximus::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, MinimusMaximus::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, MinimusMaximus::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, MinimusMaximus::D_INPUT));
		
		// logical outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, MinimusMaximus::A_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, MinimusMaximus::A_MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, MinimusMaximus::B_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, MinimusMaximus::B_MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, MinimusMaximus::C_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, MinimusMaximus::C_MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, MinimusMaximus::D_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, MinimusMaximus::D_MAX_OUTPUT));
		
		// mathematical outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, MinimusMaximus::AVG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, MinimusMaximus::MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, MinimusMaximus::MAX_OUTPUT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMinimusMaximus = Model::create<MinimusMaximus, MinimusMaximusWidget>("Count Modula", "MinimusMaximus", "Minimus Maximus", UTILITY_TAG);