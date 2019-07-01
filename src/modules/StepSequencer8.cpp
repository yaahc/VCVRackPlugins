//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME StepSequencer8
#define WIDGET_NAME StepSequencer8Widget
#define MODULE_NAME "StepSequencer8"
#define PANEL_FILE "res/StepSequencer8.svg"
#define MODEL_NAME	modelStepSequencer8

#define SEQ_NUM_SEQS	2
#define SEQ_NUM_STEPS	8

#define SEQ_STEP_INPUTS

struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_SW_PARAMS, SEQ_NUM_STEPS * 2 * SEQ_NUM_SEQS),
		ENUMS(STEP_CV_PARAMS, SEQ_NUM_STEPS * 2 * SEQ_NUM_SEQS),
		ENUMS(LENGTH_PARAMS, SEQ_NUM_SEQS),
		ENUMS(CV_PARAMS, SEQ_NUM_SEQS),
		ENUMS(MUTE_PARAMS, SEQ_NUM_SEQS * 2),
		ENUMS(RANGE_PARAMS, SEQ_NUM_SEQS * 2),
		ENUMS(RANGE_SW_PARAMS, SEQ_NUM_SEQS * 2),
		ENUMS(MODE_PARAMS, SEQ_NUM_SEQS),
		NUM_PARAMS
	};
	
	enum InputIds {
		ENUMS(RUN_INPUTS, SEQ_NUM_SEQS),
		ENUMS(CLOCK_INPUTS, SEQ_NUM_SEQS),
		ENUMS(RESET_INPUTS, SEQ_NUM_SEQS),
		ENUMS(LENCV_INPUTS, SEQ_NUM_SEQS),
		ENUMS(STEP_INPUTS, SEQ_NUM_STEPS),
		ENUMS(DIRCV_INPUTS, SEQ_NUM_STEPS),
		NUM_INPUTS
	};
	
	enum OutputIds {
		ENUMS(TRIG_OUTPUTS, SEQ_NUM_SEQS * 2),
		ENUMS(GATE_OUTPUTS, SEQ_NUM_SEQS * 2),
		ENUMS(CV_OUTPUTS, 2 * SEQ_NUM_SEQS),
		ENUMS(CVI_OUTPUTS, 2 * SEQ_NUM_SEQS),
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS * SEQ_NUM_SEQS),
		ENUMS(TRIG_LIGHTS, SEQ_NUM_SEQS * 2),
		ENUMS(GATE_LIGHTS, SEQ_NUM_SEQS * 2),
		ENUMS(DIR_LIGHTS, SEQ_NUM_SEQS * 3),
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS * SEQ_NUM_SEQS),
		NUM_LIGHTS
	};
	
	enum Directions {
		FORWARD,
		PENDULUM,
		REVERSE
	};
	
	GateProcessor gateClock[SEQ_NUM_SEQS];
	GateProcessor gateReset[SEQ_NUM_SEQS];
	GateProcessor gateRun[SEQ_NUM_SEQS];
	
	int count[SEQ_NUM_SEQS]; 
	int length[SEQ_NUM_SEQS];
	int direction[SEQ_NUM_SEQS];
	int directionMode[SEQ_NUM_SEQS];
	
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);
	float stepInputVoltages[SEQ_NUM_STEPS];
	
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char textBuffer[100];
		
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			// length params
			sprintf(textBuffer, "Channel %d length", r + 1);
			configParam(LENGTH_PARAMS + r, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), textBuffer);
			
			//  mode params
			sprintf(textBuffer, "Channel %d direction", r + 1);
			configParam(MODE_PARAMS + r, 0.0f, 2.0, 0.0f, textBuffer);
			
			// row lights and switches
			int sw = 0;
			int k = 0;
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				sprintf(textBuffer, "Select %s", r == 0 ? "Trig A/ Trig B" : "Trig E/ Trig F");
				configParam(STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++, 0.0f, 2.0f, 1.0f, textBuffer);
				
				sprintf(textBuffer, "Select %s", r == 0 ? "Trig C/ Gate D" : "Trig G/ get H");
				configParam(STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++, 0.0f, 2.0f, 1.0f, textBuffer);

				sprintf(textBuffer, "Step value %s", r == 0 ? "(CV1/2)" : " (CV3)");
				configParam(STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++, 0.0f, 8.0f, 0.0f, textBuffer);
			}

			// add second row of knobs to channel 2
			if (r >  0) {
				for (int s = 0; s < SEQ_NUM_STEPS; s++) {
					configParam(STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++, 0.0f, 8.0f, 0.0f, "Step value (CV4)");
				}
			}
			
			// mute buttons
			char muteText[4][20] = {"Mute Trig A/Trig C",
									"Mute Trig B/Gate D",
									"Mute Trig E/Trig G",
									"Mute Trig F/Gate H"};
								
			for (int i = 0; i < 2; i++) {
				configParam(MUTE_PARAMS + + (r * 2) + i, 0.0f, 1.0f, 0.0f, muteText[i + (r * 2)]);
			}
			
			// range controls
			for (int i = 0; i < 2; i++) {
				
				// use a single pot on the 1st row of the channel 1 and a switch on channel 2
				if (r == 0) {
					if (i == 0)
						configParam(RANGE_PARAMS + (r * 2) + i, 0.0f, 1.0f, 1.0f, "Range", " %", 0.0f, 100.0f, 0.0f);
				}
				else
					configParam(RANGE_SW_PARAMS + + (r * 2) + i, 0.0f, 2.0f, 0.0f, "Scale");

				// 2nd row on channel 2
				if (r > 0)
					configParam(RANGE_SW_PARAMS + + (r * 2) + i, 0.0f, 2.0f, 0.0f, "Scale");
			}			
		}
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_t *currentStep = json_array();
		json_t *dir = json_array();
		
		for (int i = 0; i < SEQ_NUM_SEQS; i++) {
			json_array_insert_new(currentStep, i, json_integer(count[i]));
			json_array_insert_new(dir, i, json_integer(direction[i]));
		}
		
		json_object_set_new(root, "currentStep", currentStep);
		json_object_set_new(root, "direction", dir);

		return root;
	}

	void dataFromJson(json_t *root) override {
		
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *dir = json_object_get(root, "direction");
		
		for (int i = 0; i < SEQ_NUM_SEQS; i++) {
			if (currentStep) {
				json_t *v = json_array_get(currentStep, i);
				if (v)
					count[i] = json_integer_value(v);
			}
			
			if (dir) {
				json_t *v = json_array_get(dir, i);
				if (v)
					direction[i] = json_integer_value(v);
			}
		}
	}

	void onReset() override {
		
		for (int i = 0; i < SEQ_NUM_SEQS; i++) {
			gateClock[i].reset();
			gateReset[i].reset();
			gateRun[i].reset();
			count[i] = 0;
			length[i] = SEQ_NUM_STEPS;
			direction[i] = FORWARD;
			directionMode[i] = FORWARD;
		}
	}

	void onRandomize() override {
		// don't randomize mute buttons
		for (int i = 0; i < SEQ_NUM_SEQS * 2; i++) {
			params[MUTE_PARAMS + i].setValue(0.0f);
		}
	}
	
	float getScale(float range) {
		
		switch ((int)(range)) {
			case 2:
				return 0.25f; // 2 volts
			case 1:
				return 0.5f; // 4 volts
			case 0:
			default:
				return 1.0f; // 8 volts
		}
	}
	
	int recalcDirection(int r) {
		
		switch (directionMode[r]) {
			case PENDULUM:
				return (direction[r] == FORWARD ? REVERSE : FORWARD);
			case FORWARD:
			case REVERSE:
			default:
				return directionMode[r];
		}
	}
	
	void setDirectionLight(int r) {
		lights[DIR_LIGHTS + (r * 3)].setBrightness(boolToLight(directionMode[r] == REVERSE)); 		// red
		lights[DIR_LIGHTS + (r * 3) + 1].setBrightness(boolToLight(directionMode[r] == PENDULUM) * 0.5f); 	// yellow
		lights[DIR_LIGHTS + (r * 3) + 2].setBrightness(boolToLight(directionMode[r] == FORWARD)); 	// green
	}
	
	void process(const ProcessArgs &args) override {

		// grab all the common input values up front
		float reset = 0.0f;
		float run = 10.0f;
		float clock = 0.0f;
		float f;
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			// reset input
			f = inputs[RESET_INPUTS + r].getNormalVoltage(reset);
			gateReset[r].set(f);
			reset = f;
			
			// run input
			f = inputs[RUN_INPUTS + r].getNormalVoltage(run);
			gateRun[r].set(f);
			run = f;

			// clock
			f = inputs[CLOCK_INPUTS + r].getNormalVoltage(clock); 
			gateClock[r].set(f);
			clock = f;
			
			// sequence length - jack overrides knob
			if (inputs[LENCV_INPUTS + r].isConnected()) {
				// scale the input such that 10V = step 16, 0V = step 1
				length[r] = (int)(clamp(lengthCVScale/10.0f * inputs[LENCV_INPUTS + r].getVoltage(), 0.0f, lengthCVScale)) + 1;
			}
			else {
				length[r] = (int)(params[LENGTH_PARAMS + r].getValue());
			}
			
			// set the length lights
			for(int i = 0; i < SEQ_NUM_STEPS; i++) {
				lights[LENGTH_LIGHTS + (r * SEQ_NUM_STEPS) + i].setBrightness(boolToLight(i < length[r]));
			}
			
			// direction - jack overrides the switch
			if (inputs[DIRCV_INPUTS + r].isConnected()) {
				float dirCV = clamp(inputs[DIRCV_INPUTS + r].getVoltage(), 0.0f, 10.0f);
				if (dirCV < 2.0f)
					directionMode[r] = FORWARD;
				else if (dirCV < 4.0f)
					directionMode[r] = PENDULUM;
				else
					directionMode[r] = REVERSE;
			}
			else
				directionMode[r] = (int)(params[MODE_PARAMS + r].getValue());

			setDirectionLight(r);			
		}
		
		// pre-process the step inputs to determine what values to use
		// if input one is a polyphonic connection, we spread the channels across any unused inputs.
		int numChannels = inputs[STEP_INPUTS].getChannels();
		bool isPoly = numChannels > 1; 
		int j = 0;
		for (int i = 0; i < SEQ_NUM_STEPS; i ++) {
			if (i > 0 && inputs[STEP_INPUTS + i].isConnected())
				stepInputVoltages[i] = inputs[STEP_INPUTS + i].getVoltage();
			else {
				if (isPoly) {
					stepInputVoltages[i] = inputs[STEP_INPUTS].getVoltage(j++);
					if (j >= numChannels)
						j = 0;
				}
				else 
					stepInputVoltages[i] = inputs[STEP_INPUTS + i].getNormalVoltage(8.0f);
			}
		}

		// now process the steps for each row as required
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			// which mode are we using? jack overrides the switch
			int nextDir = recalcDirection(r);
			
			// switch non-pendulum mode right away
			if (directionMode[r] != PENDULUM)
				direction[r] = nextDir;
			
			if (gateReset[r].leadingEdge()) {
				
				// restart pendulum at forward stage
				if (directionMode[r] == PENDULUM)
					direction[r]  = nextDir = FORWARD;
				
				// reset the count according to the next direction
				switch (nextDir) {
					case FORWARD:
						count[r] = 0;
						break;
					case REVERSE:
						count[r] = SEQ_NUM_STEPS;
						break;
				}
				
				direction[r] = nextDir;
			}

			bool running = gateRun[r].high();
			if (running) {
				// advance count on positive clock edge
				if (gateClock[r].leadingEdge()){
					if (direction[r] == FORWARD) {
						count[r]++;
						
						if (count[r] > length[r]) {
							if (nextDir == FORWARD)
								count[r] = 1;
							else {
								// in pendulum mode we change direction here
								count[r]--;
								direction[r] = nextDir;
							}
						}
					}
					else {
						count[r]--;
						
						if (count[r] < 1) {
							if (nextDir == REVERSE)
								count[r] = length[r];
							else {
								// in pendulum mode we change direction here
								count[r]++;
								direction[r] = nextDir;
							}
						}
					}
				}
			}
			
			// now process the lights and outputs
			bool trig1 = false, trig2 = false;
			bool gate1 = false, gate2 = false;
			float cv1 = 0.0f, cv2 = 0.0f;
			float scale1 = 1.0f, scale2 = 1.0f;
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// set step lights here
				bool stepActive = (c + 1 == count[r]);
				lights[STEP_LIGHTS + (r * SEQ_NUM_STEPS) + c].setBrightness(boolToLight(stepActive));
				
				// now determine the output values
				if (stepActive) {
					// triggers
					switch ((int)(params[STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + (c * 2)].getValue())) {
						case 0: // lower output
							trig1 = false;
							trig2 = true;
							break;
						case 2: // upper output
							trig1 = true;
							trig2 = false;
							break;				
						default: // off
							trig1 = false;
							trig2 = false;
							break;
					}
					
					// gates
					switch ((int)(params[STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + (c * 2) +1].getValue())) {
						case 0: // lower output
							gate1 = false;
							gate2 = true;
							break;
						case 2: // upper output
							gate1 = true;
							gate2 = false;
							break;				
						default: // off
							gate1 = false;
							gate2 = false;
							break;
					}
					
					// determine which scale to use
					if (r == 0) {
						scale1 = scale2 = params[RANGE_PARAMS + (r * 2)].getValue();
					}
					else {
						scale1 = getScale(params[RANGE_SW_PARAMS + (r * 2)].getValue());
						scale2 = getScale(params[RANGE_SW_PARAMS + (r * 2) + 1].getValue());
					}
					
					// now grab the cv for row 1
					cv1 = params[STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + c].getValue() * scale1;
					
					if (r == 0) {
						// row 2 on channel 1 is the CV/audio inputs attenuated by the knobs
						cv2 = stepInputVoltages[c] * (params[STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + c].getValue()/ 8.0f) * scale2;
					}
					else {
						cv2 = params[STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + SEQ_NUM_STEPS + c].getValue() * scale2;
					}
				}
			}

			// trig outputs follow clock width
			trig1 &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f));
			trig2 &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f));

			// gate2 output follows step widths, gate 1 follows clock width
			gate1 &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f));
			gate2 &= (running && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f));

			// set the outputs accordingly
			outputs[TRIG_OUTPUTS + (r * 2)].setVoltage(boolToGate(trig1));	
			outputs[TRIG_OUTPUTS + (r * 2) + 1].setVoltage(boolToGate(trig2));

			outputs[GATE_OUTPUTS + (r * 2)].setVoltage(boolToGate(gate1));	
			outputs[GATE_OUTPUTS + (r * 2) + 1].setVoltage(boolToGate(gate2));
			
			outputs[CV_OUTPUTS + (r * 2)].setVoltage(cv1);
			outputs[CV_OUTPUTS + (r * 2) + 1].setVoltage(cv2);

			outputs[CVI_OUTPUTS + (r * 2)].setVoltage(-cv1);
			outputs[CVI_OUTPUTS + (r * 2) + 1].setVoltage(-cv2);
			
			lights[TRIG_LIGHTS + (r * 2)].setBrightness(boolToLight(trig1));	
			lights[TRIG_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(trig2));
			lights[GATE_LIGHTS + (r * 2)].setBrightness(boolToLight(gate1));	
			lights[GATE_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(gate2));
		}
	}

};

struct WIDGET_NAME : ModuleWidget {
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, PANEL_FILE)));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// add everything row by row
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			// run input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (r * 4)]), module, STRUCT_NAME::RUN_INPUTS + r));

			// reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (r * 4)]), module, STRUCT_NAME::RESET_INPUTS + r));
			
			// clock input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3 + (r * 4)]), module, STRUCT_NAME::CLOCK_INPUTS + r));
					
			// length CV input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4 + (r * 4)]), module, STRUCT_NAME::LENCV_INPUTS + r));

			// direction CV input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_ROWS8[STD_ROW1 + (r * 4)]), module, STRUCT_NAME::DIRCV_INPUTS + r));
			
			// direction light
			addChild(createLightCentered<MediumLight<CountModulaLightRYG>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] - 1, STD_HALF_ROWS8(STD_ROW1 + (r * 4)) - 5), module, STRUCT_NAME::DIR_LIGHTS + (r * 3)));
			
			
			// length & mode params
			switch (r % SEQ_NUM_SEQS) {
				case 0:
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_ROWS8[STD_ROW2 + (r * 4)]), module, STRUCT_NAME::MODE_PARAMS + r));
					addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_HALF_ROWS8(STD_ROW3 + (r * 4))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 1:
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_ROWS8[STD_ROW2 + (r * 4)]), module, STRUCT_NAME::MODE_PARAMS + r));
					addParam(createParamCentered<CountModulaRotarySwitchOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_HALF_ROWS8(STD_ROW3 + (r * 4))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
			}
			
			// row lights and switches
			int i = 0;
			int sw = 0;
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW2 + (r * 4)] + 10), module, STRUCT_NAME::STEP_LIGHTS + (r * SEQ_NUM_STEPS) + i));
				addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)], STD_ROWS8[STD_ROW1 + (r * 4)] - 13), module, STRUCT_NAME::LENGTH_LIGHTS + (r * SEQ_NUM_STEPS) + i++));

				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)], STD_HALF_ROWS8(STD_ROW1 + (r * 4))), module, STRUCT_NAME:: STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++));
				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_HALF_ROWS8(STD_ROW1 + (r * 4))), module, STRUCT_NAME:: STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++));
			}

			// CV knob row 1
			int k = 0;
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				switch (r % SEQ_NUM_SEQS) {
					case 0:
						addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4)]), module, STRUCT_NAME::STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++));
					break;
				case 1:
						addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4)]), module, STRUCT_NAME::STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++));
					break;
				}
			}
			
			// CV knob row 2 (step inputs on 1st sequencer)
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				switch (r % SEQ_NUM_SEQS) {
					case 0:
						addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW4 + (r * 4)]), module, STRUCT_NAME::STEP_INPUTS + s));
						break;
					case 1:
						addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW4 + (r * 4)]), module, STRUCT_NAME::STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++));
						break;
				}
			}
			
			// output lights, mute buttons
			for (int i = 0; i < 2; i++) {
				addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i]), module, STRUCT_NAME::MUTE_PARAMS + (r * 2) + i));
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 1] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i] - 10), module, STRUCT_NAME::TRIG_LIGHTS + (r * 2) + i));
				addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 1] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i] + 10), module, STRUCT_NAME::GATE_LIGHTS + (r * 2) + i));
			}
			
			// range controls
			for (int i = 0; i < 2; i++) {
				
				// use a single pot on the 1st row of the channel 1 and a switch on channel 2
				if (r == 0) {
					if (i == 0)
						addParam(createParamCentered<CountModulaKnobGrey>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_HALF_ROWS8(STD_ROW3 + (r * 4) + i)), module, STRUCT_NAME::RANGE_PARAMS + (r * 2) + i));
				}
				else
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::RANGE_SW_PARAMS + (r * 2) + i));

				// 2nd row on channel 2
				if (r > 0)
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::RANGE_SW_PARAMS + (r * 2) + i));
			}
			
			// trig output jacks TA/TB
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 2] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i]), module, STRUCT_NAME::TRIG_OUTPUTS + (r * 2) + i));
			
			// gate output jacks GC/GD
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + (SEQ_NUM_STEPS * 2) + 2], STD_ROWS8[STD_ROW1 + (r * 4) + i]), module, STRUCT_NAME::GATE_OUTPUTS + (r * 2) + i));

			// cv output jacks A/B
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 2] + 15, STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::CV_OUTPUTS + (r * 2) + i));

			// cv output jacks IA/IB
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + (SEQ_NUM_STEPS * 2) + 2], STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::CVI_OUTPUTS + (r * 2) + i));
		}
	}
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);