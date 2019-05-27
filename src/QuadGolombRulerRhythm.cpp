#include <string.h>
#include "FrozenWasteland.hpp"
#include "ui/knobs.hpp"

#define NUM_RULERS 10
#define MAX_DIVISIONS 6
#define TRACK_COUNT 4
#define MAX_STEPS 18

struct QuadGolombRulerRhythm : Module {
	enum ParamIds {
		STEPS_1_PARAM,
		DIVISIONS_1_PARAM,
		OFFSET_1_PARAM,
		PAD_1_PARAM,
		ACCENTS_1_PARAM,
		ACCENT_ROTATE_1_PARAM,
		STEPS_2_PARAM,
		DIVISIONS_2_PARAM,
		OFFSET_2_PARAM,
		PAD_2_PARAM,
		ACCENTS_2_PARAM,
		ACCENT_ROTATE_2_PARAM,
		STEPS_3_PARAM,
		DIVISIONS_3_PARAM,
		OFFSET_3_PARAM,
		PAD_3_PARAM,
		ACCENTS_3_PARAM,
		ACCENT_ROTATE_3_PARAM,
		STEPS_4_PARAM,
		DIVISIONS_4_PARAM,
		OFFSET_4_PARAM,
		PAD_4_PARAM,
		ACCENTS_4_PARAM,
		ACCENT_ROTATE_4_PARAM,
		CHAIN_MODE_PARAM,	
		CONSTANT_TIME_MODE_PARAM,		
		NUM_PARAMS
	};
	enum InputIds {
		STEPS_1_INPUT,
		DIVISIONS_1_INPUT,
		OFFSET_1_INPUT,
		PAD_1_INPUT,
		ACCENTS_1_INPUT,
		ACCENT_ROTATE_1_INPUT,
		START_1_INPUT,
		STEPS_2_INPUT,
		DIVISIONS_2_INPUT,
		OFFSET_2_INPUT,
		PAD_2_INPUT,
		ACCENTS_2_INPUT,
		ACCENT_ROTATE_2_INPUT,
		START_2_INPUT,
		STEPS_3_INPUT,
		DIVISIONS_3_INPUT,
		OFFSET_3_INPUT,
		PAD_3_INPUT,
		ACCENTS_3_INPUT,
		ACCENT_ROTATE_3_INPUT,
		START_3_INPUT,
		STEPS_4_INPUT,
		DIVISIONS_4_INPUT,
		OFFSET_4_INPUT,
		PAD_4_INPUT,
		ACCENTS_4_INPUT,
		ACCENT_ROTATE_4_INPUT,
		START_4_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		MUTE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_1,
		ACCENT_OUTPUT_1,
		EOC_OUTPUT_1,
		OUTPUT_2,
		ACCENT_OUTPUT_2,
		EOC_OUTPUT_2,
		OUTPUT_3,
		ACCENT_OUTPUT_3,
		EOC_OUTPUT_3,
		OUTPUT_4,
		ACCENT_OUTPUT_4,
		EOC_OUTPUT_4,
		NUM_OUTPUTS
	};
	enum LightIds {
		CHAIN_MODE_NONE_LIGHT,
		CHAIN_MODE_BOSS_LIGHT,
		CHAIN_MODE_EMPLOYEE_LIGHT,
		MUTED_LIGHT,
		NUM_LIGHTS
	};
	enum ChainModes {
		CHAIN_MODE_NONE,
		CHAIN_MODE_BOSS,
		CHAIN_MODE_EMPLOYEE
	};


	bool beatMatrix[TRACK_COUNT][MAX_STEPS];
	bool accentMatrix[TRACK_COUNT][MAX_STEPS];
	int beatIndex[TRACK_COUNT];
	int stepsCount[TRACK_COUNT];
	float stepDuration[TRACK_COUNT];
	float lastStepTime[TRACK_COUNT];
	float maxStepCount;
	float masterStepCount;


	const int rulerOrders[NUM_RULERS] = {1,2,3,4,5,5,6,6,6,6};
	const int rulerLengths[NUM_RULERS] = {0,1,3,6,11,11,17,17,17,17};
	const int rulers[NUM_RULERS][MAX_DIVISIONS] = {{0},
												   {0,1},
												   {0,1,3},
												   {0,1,4,6},
												   {0,1,4,9,11},
												   {0,2,7,8,11},
												   {0,1,4,10,12,17},
												   {0,1,4,10,15,17},
												   {0,1,8,11,13,17},
												   {0,1,8,12,14,17}};

	bool running[TRACK_COUNT];
	int chainMode = 0;
	bool initialized = false;
	bool muted = false;
	bool constantTime = false;
	int masterTrack = 0;

	float time = 0.0;
	float duration = 0.0;
	bool secondClockReceived = false;

	dsp::SchmittTrigger clockTrigger,resetTrigger,chainModeTrigger,constantTimeTrigger,muteTrigger,startTrigger[TRACK_COUNT];
	dsp::PulseGenerator beatPulse[TRACK_COUNT],accentPulse[TRACK_COUNT],eocPulse[TRACK_COUNT];


	QuadGolombRulerRhythm()  {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);			
		configParam(STEPS_1_PARAM, 0.0, 18.2, 18.0);
		configParam(DIVISIONS_1_PARAM, 0.0, 10.2, 2.0);
		configParam(OFFSET_1_PARAM, 0.0, 17.2, 0.0);
		configParam(PAD_1_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENTS_1_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENT_ROTATE_1_PARAM, 0.0, 17.2, 0.0);
		configParam(STEPS_2_PARAM, 0.0, 18.0, 18.0);
		configParam(DIVISIONS_2_PARAM, 0.0, 10.2, 2.0);
		configParam(OFFSET_2_PARAM, 0.0, 17.2, 0.0);
		configParam(PAD_2_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENTS_2_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENT_ROTATE_2_PARAM, 0.0, 17.2, 0.0);
		configParam(STEPS_3_PARAM, 0.0, 18.2, 18.0);
		configParam(DIVISIONS_3_PARAM, 0.0, 10.2, 2.0);
		configParam(OFFSET_3_PARAM, 0.0, 17.2, 0.0);
		configParam(PAD_3_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENTS_3_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENT_ROTATE_3_PARAM, 0.0, 17.2, 0.0);
		configParam(STEPS_4_PARAM, 0.0, 18.2, 18.0);
		configParam(DIVISIONS_4_PARAM, 0.0, 10.2, 2.0);
		configParam(OFFSET_4_PARAM, 0.0, 17.2, 0.0);
		configParam(PAD_4_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENTS_4_PARAM, 0.0, 17.2, 0.0);
		configParam(ACCENT_ROTATE_4_PARAM, 0.0, 17.2, 0.0);
		configParam(CHAIN_MODE_PARAM, 0.0, 1.0, 0.0);
		configParam(CONSTANT_TIME_MODE_PARAM, 0.0, 1.0, 0.0);

		for(unsigned i = 0; i < TRACK_COUNT; i++) {
			beatIndex[i] = 0;
			stepsCount[i] = MAX_STEPS;
			lastStepTime[i] = 0.0;
			stepDuration[i] = 0.0;
			running[i] = true;
			for(unsigned j = 0; j < MAX_STEPS; j++) {
				beatMatrix[i][j] = false;
				accentMatrix[i][j] = false;
			}
		}		
	}
	void process(const ProcessArgs &args) override;


	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "constantTime", json_integer((bool) constantTime));
		json_object_set_new(rootJ, "masterTrack", json_integer((int) masterTrack));
		json_object_set_new(rootJ, "chainMode", json_integer((int) chainMode));
		json_object_set_new(rootJ, "muted", json_integer((bool) muted));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *ctJ = json_object_get(rootJ, "constantTime");
		if (ctJ)
			constantTime = json_integer_value(ctJ);

		json_t *mtJ = json_object_get(rootJ, "masterTrack");
		if (mtJ)
			masterTrack = json_integer_value(mtJ);

		json_t *cmJ = json_object_get(rootJ, "chainMode");
		if (cmJ)
			chainMode = json_integer_value(cmJ);

		json_t *mutedJ = json_object_get(rootJ, "muted");
		if (mutedJ)
			muted = json_integer_value(mutedJ);
	}

	void setRunningState() {
		for(int trackNumber=0;trackNumber<TRACK_COUNT;trackNumber++)
		{
			if(chainMode == CHAIN_MODE_EMPLOYEE && inputs[(trackNumber * 7) + 6].isConnected()) { //START Input needs to be active
				running[trackNumber] = false;
			}
			else {
				running[trackNumber] = true;
			}
		}
	}

	void advanceBeat(int trackNumber) {
		beatIndex[trackNumber]++;
		lastStepTime[trackNumber] = 0.0;		
					
		//End of Cycle
		if(beatIndex[trackNumber] >= stepsCount[trackNumber]) {
			beatIndex[trackNumber] = 0;
			eocPulse[trackNumber].trigger(1e-3);
			//If in a chain mode, stop running until start trigger received
			if(chainMode != CHAIN_MODE_NONE && inputs[(trackNumber * 7) + 6].isConnected()) { //START Input needs to be active
				running[trackNumber] = false;
			}
		}
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};



void QuadGolombRulerRhythm::process(const ProcessArgs &args) {

	int accentLevelArray[MAX_STEPS];
	int beatLocation[MAX_STEPS];

	//Set startup state
	if(!initialized) {
		setRunningState();
		initialized = true;
	}

	// Modes
	if (constantTimeTrigger.process(params[CONSTANT_TIME_MODE_PARAM].getValue())) {
		masterTrack = (masterTrack + 1) % 5;
		constantTime = masterTrack > 0;
		for(int trackNumber=0;trackNumber<TRACK_COUNT;trackNumber++) {
			beatIndex[trackNumber] = 0;
		}
		setRunningState();
	}
	

	if (chainModeTrigger.process(params[CHAIN_MODE_PARAM].getValue())) {
		chainMode = (chainMode + 1) % 3;
		setRunningState();
	}
	lights[CHAIN_MODE_NONE_LIGHT].value = chainMode == CHAIN_MODE_NONE ? 1.0 : 0.0;
	lights[CHAIN_MODE_BOSS_LIGHT].value = chainMode == CHAIN_MODE_BOSS ? 1.0 : 0.0;
	lights[CHAIN_MODE_EMPLOYEE_LIGHT].value = chainMode == CHAIN_MODE_EMPLOYEE ? 1.0 : 0.0;

	lights[MUTED_LIGHT].value = muted ? 1.0 : 0.0;

	maxStepCount = 0;
	masterStepCount = 0;

	for(int trackNumber=0;trackNumber<TRACK_COUNT;trackNumber++) {
		//clear out the matrix and levels
		for(int j=0;j<MAX_STEPS;j++)
		{
			// beatMatrix[trackNumber][j] = false; 
			accentMatrix[trackNumber][j] = false;
			accentLevelArray[j] = 0;	
			beatLocation[j] = 0;
		}

		float stepsCountf = params[trackNumber * 6].getValue();
		if(inputs[trackNumber * 7].isConnected()) {
			stepsCountf += inputs[trackNumber * 7].getVoltage() * 1.8;
		}
		stepsCountf = clamp(stepsCountf,0.0f,18.0f);

		float divisionf = params[(trackNumber * 6) + 1].getValue();
		if(inputs[(trackNumber * 7) + 1].isConnected()) {
			divisionf += inputs[(trackNumber * 7) + 1].getVoltage() * 1.7;
		}		
		divisionf = clamp(divisionf,0.0f,(float)(NUM_RULERS-1));

		float offsetf = params[(trackNumber * 6) + 2].getValue();
		if(inputs[(trackNumber * 7) + 2].isConnected()) {
			offsetf += inputs[(trackNumber * 7) + 2].getVoltage() * 1.7;
		}	
		offsetf = clamp(offsetf,0.0f,17.0f);

		float padf = params[trackNumber * 6 + 3].getValue();
		if(inputs[trackNumber * 7 + 3].isConnected()) {
			padf += inputs[trackNumber * 7 + 3].getVoltage() * 1.7;
		}
		padf = clamp(padf,0.0f,stepsCountf - divisionf);


		//Use this to reduce range of accent params/inputs so the range of motion of knob/modulation is more useful.
		float divisionScale = 1;
		if(stepsCountf > 0) {
			divisionScale = divisionf / stepsCountf;
		}		

		float accentDivisionf = params[(trackNumber * 6) + 4].getValue() * divisionScale;
		if(inputs[(trackNumber * 7) + 4].isConnected()) {
			accentDivisionf += inputs[(trackNumber * 7) + 4].getVoltage() * divisionScale;
		}
		accentDivisionf = clamp(accentDivisionf,0.0f,divisionf);

		float accentRotationf = params[(trackNumber * 6) + 5].getValue() * divisionScale;
		if(inputs[(trackNumber * 7) + 5].isConnected()) {
			accentRotationf += inputs[(trackNumber * 7) + 5].getVoltage() * divisionScale;
		}
		if(divisionf > 0) {
			accentRotationf = clamp(accentRotationf,0.0f,divisionf-1);			
		} else {
			accentRotationf = 0;
		}

		if(stepsCountf > maxStepCount)
			maxStepCount = stepsCountf;
		if(trackNumber == masterTrack - 1)
			masterStepCount = stepsCountf;
		else
			masterStepCount = maxStepCount;

		stepsCount[trackNumber] = int(stepsCountf);
		int division = int(divisionf);
		int offset = int(offsetf);		
		int pad = int(padf);
		int accentDivision = int(accentDivisionf);
		int accentRotation = int(accentRotationf);


		//Calculate Beats if there are divisions to calcuate for	
		//Ruler Lengths and orders are 0 based, which is why I add 1 here. 	
		//Yes, I could add 1 to length in the constant array, but I want that to match wikipedia definition				
		if(stepsCount[trackNumber] > 0 && division > 0) {
			int rulerToUse = division - 1;
			int actualStepCount = stepsCount[trackNumber] - pad;
			while(rulerLengths[rulerToUse] + 1 > actualStepCount && rulerToUse >= 	0) {
				rulerToUse -=1;
			} 
			
			//Multiply beats so that low division beats fill out entire pattern
			int spaceMultiplier = (actualStepCount / (rulerLengths[rulerToUse] + 1)) + 1;
			if(actualStepCount % (rulerLengths[rulerToUse] + 1) == 0) {
				spaceMultiplier -=1;
			}			

			
			//Set all beats to false
			for(int j=0;j<actualStepCount;j++)
			{
				beatMatrix[trackNumber][j] = false; 			
			}

			for (int j = 0; j < rulerOrders[rulerToUse];j++)
			{
				int divisionLocation = rulers[rulerToUse][j] * spaceMultiplier;
				divisionLocation +=pad;
				if(j > 0) {
					divisionLocation -=1;
				}
				beatMatrix[trackNumber][(divisionLocation + offset) % stepsCount[trackNumber]] = true;
	            beatLocation[j] = (divisionLocation + offset) % stepsCount[trackNumber];	            
			}

		

	        //Calculate Accents
	        int level = 0;
	        int restsLeft = std::max(0,rulerOrders[rulerToUse]-accentDivision); // just make sure no negatives
	        do {
				accentLevelArray[level] = std::min(restsLeft,accentDivision);
				restsLeft = restsLeft - accentDivision;
				level += 1;
			} while (restsLeft > 0 && level < MAX_STEPS);

			int tempIndex =0;
			for (int j = 0; j < accentDivision; j++) {
	            accentMatrix[trackNumber][beatLocation[(tempIndex + accentRotation) % rulerOrders[rulerToUse]]] = true;
	            tempIndex++;
	            for (int k = 0; k < MAX_STEPS; k++) {
	                if (accentLevelArray[k] > j) {
	                    tempIndex++;
	                }
	            }
	        }	
        }	
	}

	if(inputs[RESET_INPUT].isConnected()) {
		if(resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
			for(int trackNumber=0;trackNumber<TRACK_COUNT;trackNumber++)
			{
				beatIndex[trackNumber] = 0;
			}
			setRunningState();
		}
	}

	if(inputs[MUTE_INPUT].isConnected()) {
		if(muteTrigger.process(inputs[MUTE_INPUT].getVoltage())) {
			muted = !muted;
		}
	}

	//See if need to start up
	for(int trackNumber=0;trackNumber < TRACK_COUNT;trackNumber++) {
		if(chainMode != CHAIN_MODE_NONE && inputs[(trackNumber * 7) + 6].isConnected() && !running[trackNumber]) {
			if(startTrigger[trackNumber].process(inputs[(trackNumber * 7) + 6].getVoltage())) {
				running[trackNumber] = true;
			}
		}
	}

	//Advance beat on trigger if not in constant time mode
	float timeAdvance =1.0 / args.sampleRate;
    time += timeAdvance;
	if(inputs[CLOCK_INPUT].isConnected()) {
		if(clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			if(secondClockReceived) {
				duration = time;
			}
			time = 0;
			secondClockReceived = true;			

			for(int trackNumber=0;trackNumber < TRACK_COUNT;trackNumber++)
			{
				if(running[trackNumber]) {
					if(!constantTime) {
						advanceBeat(trackNumber);
					}					
				}
			}
		}

		bool resyncAll = false;
		//For constant time, don't rely on clock trigger to advance beat, use time
		for(int trackNumber=0;trackNumber < TRACK_COUNT;trackNumber++) {
			if(stepsCount[trackNumber] > 0)
				stepDuration[trackNumber] = duration * masterStepCount / (float)stepsCount[trackNumber];

			if(running[trackNumber]) {
				lastStepTime[trackNumber] +=timeAdvance;
				if(constantTime && stepDuration[trackNumber] > 0.0 && lastStepTime[trackNumber] >= stepDuration[trackNumber]) {
					advanceBeat(trackNumber);
					if(stepsCount[trackNumber] >= (int)maxStepCount && beatIndex[trackNumber] == 0) {
						resyncAll = true;
					}
				}					
			}
		}
		if(resyncAll) {
			for(int trackNumber=0;trackNumber < TRACK_COUNT;trackNumber++) {
				lastStepTime[trackNumber] = 0;
				beatIndex[trackNumber] = 0;
			}
		}
	}

	// Set output to current state		
	for(int trackNumber=0;trackNumber<TRACK_COUNT;trackNumber++) {
		float outputValue = (lastStepTime[trackNumber] < stepDuration[trackNumber] / 2) ? 10.0 : 0.0;
		//Send out beat
		if(beatMatrix[trackNumber][beatIndex[trackNumber]] == true && running[trackNumber] && !muted) {
			outputs[trackNumber * 3].setVoltage(outputValue);	
		} else {
			outputs[trackNumber * 3].setVoltage(0.0);	
		}
		//send out accent
		if(accentMatrix[trackNumber][beatIndex[trackNumber]] == true && running[trackNumber] && !muted) {
			outputs[trackNumber * 3 + 1].setVoltage(outputValue);	
		} else {
			outputs[trackNumber * 3 + 1].setVoltage(0.0);	
		}
		//Send out End of Cycle
		outputs[(trackNumber * 3) + 2].setVoltage(eocPulse[trackNumber].process(1.0 / args.sampleRate) ? 10.0 : 0);	
	}

}

struct QBRBeatDisplay : TransparentWidget {
	QuadGolombRulerRhythm *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	QBRBeatDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/01 Digit.ttf"));
	}

	void drawBox(const DrawArgs &args, float stepNumber, float trackNumber,bool isBeat,bool isAccent,bool isCurrent) {
		
		//nvgSave(args.vg);
		//Rect b = Rect(Vec(0, 0), box.size);
		//nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
		
		float boxX = stepNumber * 22.5;
		float boxY = trackNumber * 22.5;

		float opacity = 0x80; // Testing using opacity for accents

		if(isAccent) {
			opacity = 0xff;
		}


		NVGcolor strokeColor = nvgRGBA(0, 0xe0, 0xef, 0xff);
		NVGcolor fillColor = nvgRGBA(0,0xe0,0xef,opacity);
		if(isCurrent)
		{
			strokeColor = nvgRGBA(0x2f, 0xf0, 0, 0xff);
			fillColor = nvgRGBA(0x2f,0xf0,0,opacity);			
		}

		nvgStrokeColor(args.vg, strokeColor);
		nvgStrokeWidth(args.vg, 1.0);
		nvgBeginPath(args.vg);
		nvgRect(args.vg,boxX,boxY,21,21.0);
		nvgStroke(args.vg);
		nvgClosePath(args.vg);
		if(isBeat) {
			nvgFillColor(args.vg, fillColor);
			nvgFill(args.vg);
		}
	}

	void drawMasterTrack(const DrawArgs &args, Vec pos, int track) {
		nvgFontSize(args.vg, 20);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);

		nvgFillColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0xff));
		char text[128];
		snprintf(text, sizeof(text), " %i", track);
		nvgText(args.vg, pos.x + 8, pos.y, text, NULL);
	}

	void draw(const DrawArgs &args) override {
		if (!module)
			return;
		for(int trackNumber = 0;trackNumber < 4;trackNumber++) {
			for(int stepNumber = 0;stepNumber < module->stepsCount[trackNumber];stepNumber++) {				
				bool isBeat = module->beatMatrix[trackNumber][stepNumber];
				bool isAccent = module->accentMatrix[trackNumber][stepNumber];
				bool isCurrent = module->beatIndex[trackNumber] == stepNumber && module->running[trackNumber];				
				drawBox(args, float(stepNumber), float(trackNumber),isBeat,isAccent,isCurrent);
			}
		}

		if(module->constantTime)
			drawMasterTrack(args, Vec(box.size.x - 38, box.size.y - 78), module->masterTrack);
	}
};


struct QuadGolombRulerRhythmWidget : ModuleWidget {
	QuadGolombRulerRhythmWidget(QuadGolombRulerRhythm *module) {
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/QuadGolombRulerRhythm.svg")));


		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 12, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


		{
			QBRBeatDisplay *display = new QBRBeatDisplay();
			display->module = module;
			display->box.pos = Vec(16, 34);
			//display->box.size = Vec(box.size.x-30, 135);
			display->box.size = Vec(box.size.x-15, 351);
			addChild(display);
		}


		addParam(createParam<RoundSmallFWKnob>(Vec(22, 138), module, QuadGolombRulerRhythm::STEPS_1_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(61, 138), module, QuadGolombRulerRhythm::DIVISIONS_1_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(100, 138), module, QuadGolombRulerRhythm::OFFSET_1_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(139, 138), module, QuadGolombRulerRhythm::PAD_1_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(178, 138), module, QuadGolombRulerRhythm::ACCENTS_1_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(217, 138), module, QuadGolombRulerRhythm::ACCENT_ROTATE_1_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(22, 195), module, QuadGolombRulerRhythm::STEPS_2_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(61, 195), module, QuadGolombRulerRhythm::DIVISIONS_2_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(100, 195), module, QuadGolombRulerRhythm::OFFSET_2_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(139, 195), module, QuadGolombRulerRhythm::PAD_2_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(178, 195), module, QuadGolombRulerRhythm::ACCENTS_2_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(217, 195), module, QuadGolombRulerRhythm::ACCENT_ROTATE_2_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(22, 252), module, QuadGolombRulerRhythm::STEPS_3_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(61, 252), module, QuadGolombRulerRhythm::DIVISIONS_3_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(100, 252), module, QuadGolombRulerRhythm::OFFSET_3_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(139, 252), module, QuadGolombRulerRhythm::PAD_3_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(178, 252), module, QuadGolombRulerRhythm::ACCENTS_3_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(217, 252), module, QuadGolombRulerRhythm::ACCENT_ROTATE_3_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(22, 309), module, QuadGolombRulerRhythm::STEPS_4_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(61, 309), module, QuadGolombRulerRhythm::DIVISIONS_4_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(100, 309), module, QuadGolombRulerRhythm::OFFSET_4_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(139, 309), module, QuadGolombRulerRhythm::PAD_4_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(178, 309), module, QuadGolombRulerRhythm::ACCENTS_4_PARAM));
		addParam(createParam<RoundSmallFWKnob>(Vec(217, 309), module, QuadGolombRulerRhythm::ACCENT_ROTATE_4_PARAM));
		addParam(createParam<CKD6>(Vec(250, 285), module, QuadGolombRulerRhythm::CHAIN_MODE_PARAM));
		addParam(createParam<CKD6>(Vec(360, 285), module, QuadGolombRulerRhythm::CONSTANT_TIME_MODE_PARAM));

		addInput(createInput<PJ301MPort>(Vec(23, 163), module, QuadGolombRulerRhythm::STEPS_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(62, 163), module, QuadGolombRulerRhythm::DIVISIONS_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(101, 163), module, QuadGolombRulerRhythm::OFFSET_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(140, 163), module, QuadGolombRulerRhythm::PAD_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(179, 163), module, QuadGolombRulerRhythm::ACCENTS_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(218, 163), module, QuadGolombRulerRhythm::ACCENT_ROTATE_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(23, 220), module, QuadGolombRulerRhythm::STEPS_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(62, 220), module, QuadGolombRulerRhythm::DIVISIONS_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(101, 220), module, QuadGolombRulerRhythm::OFFSET_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(140, 220), module, QuadGolombRulerRhythm::PAD_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(179, 220), module, QuadGolombRulerRhythm::ACCENTS_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(218, 220), module, QuadGolombRulerRhythm::ACCENT_ROTATE_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(23, 277), module, QuadGolombRulerRhythm::STEPS_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(62, 277), module, QuadGolombRulerRhythm::DIVISIONS_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(101, 277), module, QuadGolombRulerRhythm::OFFSET_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(140, 277), module, QuadGolombRulerRhythm::PAD_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(179, 277), module, QuadGolombRulerRhythm::ACCENTS_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(218, 277), module, QuadGolombRulerRhythm::ACCENT_ROTATE_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(23, 334), module, QuadGolombRulerRhythm::STEPS_4_INPUT));
		addInput(createInput<PJ301MPort>(Vec(62, 334), module, QuadGolombRulerRhythm::DIVISIONS_4_INPUT));
		addInput(createInput<PJ301MPort>(Vec(101, 334), module, QuadGolombRulerRhythm::OFFSET_4_INPUT));
		addInput(createInput<PJ301MPort>(Vec(140, 334), module, QuadGolombRulerRhythm::PAD_4_INPUT));
		addInput(createInput<PJ301MPort>(Vec(179, 334), module, QuadGolombRulerRhythm::ACCENTS_4_INPUT));
		addInput(createInput<PJ301MPort>(Vec(218, 334), module, QuadGolombRulerRhythm::ACCENT_ROTATE_4_INPUT));

		addInput(createInput<PJ301MPort>(Vec(262, 343), module, QuadGolombRulerRhythm::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(Vec(302, 343), module, QuadGolombRulerRhythm::RESET_INPUT));
		addInput(createInput<PJ301MPort>(Vec(335, 343), module, QuadGolombRulerRhythm::MUTE_INPUT));

		addInput(createInput<PJ301MPort>(Vec(322, 145), module, QuadGolombRulerRhythm::START_1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(322, 175), module, QuadGolombRulerRhythm::START_2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(322, 205), module, QuadGolombRulerRhythm::START_3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(322, 235), module, QuadGolombRulerRhythm::START_4_INPUT));


		addOutput(createOutput<PJ301MPort>(Vec(255, 145), module, QuadGolombRulerRhythm::OUTPUT_1));
		addOutput(createOutput<PJ301MPort>(Vec(286, 145), module, QuadGolombRulerRhythm::ACCENT_OUTPUT_1));
		addOutput(createOutput<PJ301MPort>(Vec(354, 145), module, QuadGolombRulerRhythm::EOC_OUTPUT_1));
		addOutput(createOutput<PJ301MPort>(Vec(256, 175), module, QuadGolombRulerRhythm::OUTPUT_2));
		addOutput(createOutput<PJ301MPort>(Vec(286, 175), module, QuadGolombRulerRhythm::ACCENT_OUTPUT_2));
		addOutput(createOutput<PJ301MPort>(Vec(354, 175), module, QuadGolombRulerRhythm::EOC_OUTPUT_2));
		addOutput(createOutput<PJ301MPort>(Vec(256, 205), module, QuadGolombRulerRhythm::OUTPUT_3));
		addOutput(createOutput<PJ301MPort>(Vec(286, 205), module, QuadGolombRulerRhythm::ACCENT_OUTPUT_3));
		addOutput(createOutput<PJ301MPort>(Vec(354, 205), module, QuadGolombRulerRhythm::EOC_OUTPUT_3));
		addOutput(createOutput<PJ301MPort>(Vec(256, 235), module, QuadGolombRulerRhythm::OUTPUT_4));
		addOutput(createOutput<PJ301MPort>(Vec(286, 235), module, QuadGolombRulerRhythm::ACCENT_OUTPUT_4));
		addOutput(createOutput<PJ301MPort>(Vec(354, 235), module, QuadGolombRulerRhythm::EOC_OUTPUT_4));
		
		addChild(createLight<SmallLight<BlueLight>>(Vec(282, 285), module, QuadGolombRulerRhythm::CHAIN_MODE_NONE_LIGHT));
		addChild(createLight<SmallLight<GreenLight>>(Vec(282, 300), module, QuadGolombRulerRhythm::CHAIN_MODE_BOSS_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(Vec(282, 315), module, QuadGolombRulerRhythm::CHAIN_MODE_EMPLOYEE_LIGHT));

		addChild(createLight<LargeLight<RedLight>>(Vec(363, 347), module, QuadGolombRulerRhythm::MUTED_LIGHT));	
	}
};

Model *modelQuadGolombRulerRhythm = createModel<QuadGolombRulerRhythm, QuadGolombRulerRhythmWidget>("QuadGolombRulerRhythm");
