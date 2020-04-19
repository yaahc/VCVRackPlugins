//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Breakout Module
//	Polyphonic break out box module
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Breakout
#define PANEL_FILE "Breakout.svg"

struct Breakout : Module {
	enum ParamIds {
		CHANNEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		POLY_INPUT,
		ENUMS(RECEIVE_INPUTS, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		POLY_OUTPUT,
		ENUMS(SEND_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	int numChannels;
	float v;
	int start, end;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Breakout() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(CHANNEL_PARAM, 0.0f, 1.0f, 1.0f, "Channel range");
	
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void process(const ProcessArgs &args) override {
		
		if (inputs[POLY_INPUT].isConnected()) {	
			// how many channels are we dealing with?
			numChannels = inputs[POLY_INPUT].getChannels();
			outputs[POLY_OUTPUT].setChannels(numChannels);
		
			// first 1-8 or 9-16?
			if (params[CHANNEL_PARAM].getValue() > 0.5f) {
				start = 0;
				end = 8;
			}
			else {
				start = 8;
				end = PORT_MAX_CHANNELS;
			}
			
			if (end > numChannels)
				end = numChannels;
		
			int i = 0;
			for (int c = 0; c < numChannels; c++) {
				v = inputs[POLY_INPUT].getVoltage(c);
				
				// if we're in the channel range we want, send and receive the voltages for this channel
				if (c >= start && c < end) {
					outputs[SEND_OUTPUTS + i].setVoltage(v);
					v = inputs[RECEIVE_INPUTS + i].getNormalVoltage(v);
					i++;
				}
				
				// reconstitute the poly signal
				outputs[POLY_OUTPUT].setVoltage(v, c);
			}
		}
		else {
			outputs[POLY_OUTPUT].channels = 0;
			
			for (int i = 0; i < 8; i ++)
				outputs[SEND_OUTPUTS + i].setVoltage(0.0f);
		}
	}
};

struct BreakoutWidget : ModuleWidget {
	BreakoutWidget(Breakout *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Breakout.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// poly inputs/outputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1]-10, STD_ROWS8[STD_ROW3]), module, Breakout::POLY_INPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1]-10, STD_ROWS8[STD_ROW4]), module, Breakout::POLY_OUTPUT));
		
		// individual inputs/outputs
		for (int i = 0; i < 8; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + i]), module, Breakout::SEND_OUTPUTS + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 10, STD_ROWS8[STD_ROW1 + i]), module, Breakout::RECEIVE_INPUTS + i));
		}
		
		// channel switch
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1]-10, STD_ROWS8[STD_ROW6]), module, Breakout::CHANNEL_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Breakout *module = dynamic_cast<Breakout*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}
};

Model *modelBreakout = createModel<Breakout, BreakoutWidget>("Breakout");
