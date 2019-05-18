#include "FrozenWasteland.hpp"
#include "StateVariableFilter.h"

using namespace std;

#define BANDS 4
#define FREQUENCIES 3
#define numFilters 6

struct DamianLillard : Module {
	typedef float T;

	enum ParamIds {
		FREQ_1_CUTOFF_PARAM,
		FREQ_2_CUTOFF_PARAM,
		FREQ_3_CUTOFF_PARAM,
		FREQ_1_CV_ATTENUVERTER_PARAM,
		FREQ_2_CV_ATTENUVERTER_PARAM,
		FREQ_3_CV_ATTENUVERTER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_IN,
		FREQ_1_CUTOFF_INPUT,		
		FREQ_2_CUTOFF_INPUT,		
		FREQ_3_CUTOFF_INPUT,		
		BAND_1_RETURN_INPUT,		
		BAND_2_RETURN_INPUT,
		BAND_3_RETURN_INPUT,
		BAND_4_RETURN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		BAND_1_OUTPUT,
		BAND_2_OUTPUT,
		BAND_3_OUTPUT,
		BAND_4_OUTPUT,
		MIX_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LEARN_LIGHT,
		NUM_LIGHTS
	};
	float freq[FREQUENCIES] = {0};
	float lastFreq[FREQUENCIES] = {0};
	float output[BANDS] = {0};

    StateVariableFilterState<T> filterStates[numFilters];
    StateVariableFilterParams<T> filterParams[numFilters];


	int bandOffset = 0;

	DamianLillard() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(FREQ_1_CUTOFF_PARAM, 0, 1.0, .25);
		configParam(FREQ_2_CUTOFF_PARAM, 0, 1.0, .5);
		configParam(FREQ_3_CUTOFF_PARAM, 0, 1.0, .75);
		configParam(FREQ_1_CV_ATTENUVERTER_PARAM, -1.0, 1.0, 0);
		configParam(FREQ_2_CV_ATTENUVERTER_PARAM, -1.0, 1.0, 0);
		configParam(FREQ_3_CV_ATTENUVERTER_PARAM, -1.0, 1.0, 0);

		filterParams[0].setMode(StateVariableFilterParams<T>::Mode::LowPass);
		filterParams[1].setMode(StateVariableFilterParams<T>::Mode::HiPass);
		filterParams[2].setMode(StateVariableFilterParams<T>::Mode::LowPass);
		filterParams[3].setMode(StateVariableFilterParams<T>::Mode::HiPass);
		filterParams[4].setMode(StateVariableFilterParams<T>::Mode::LowPass);
		filterParams[5].setMode(StateVariableFilterParams<T>::Mode::HiPass);

		for (int i = 0; i < numFilters; ++i) {
	        filterParams[i].setQ(5); 	
	        filterParams[i].setFreq(T(.1));
	    }
	}

	void process(const ProcessArgs &args) override;
};

void DamianLillard::process(const ProcessArgs &args) {
	
	float signalIn = inputs[SIGNAL_IN].getVoltage()/5;
	float out = 0.0;

	const float minCutoff = 15.0;
	const float maxCutoff = 8400.0;
	
	for (int i=0; i<FREQUENCIES;i++) {
		float cutoffExp = params[FREQ_1_CUTOFF_PARAM+i].getValue() + inputs[FREQ_1_CUTOFF_INPUT+i].getVoltage() * params[FREQ_1_CV_ATTENUVERTER_PARAM+i].getValue() / 10.0f; //I'm reducing range of CV to make it more useful
		cutoffExp = clamp(cutoffExp, 0.0f, 1.0f);
		freq[i] = minCutoff * powf(maxCutoff / minCutoff, cutoffExp);

		//Prevent band overlap
		if(i>0 && freq[i] < lastFreq[i-1]) {
			freq[i] = lastFreq[i-1]+1;
		}
		if(i<FREQUENCIES-1 && freq[i] > lastFreq[i+1]) {
			freq[i] = lastFreq[i+1]-1;
		}

		if(freq[i] != lastFreq[i]) {
			float Fc = freq[i] / args.sampleRate;
			filterParams[i*2].setFreq(T(Fc));
			filterParams[i*2 + 1].setFreq(T(Fc));
			lastFreq[i] = freq[i];
		}
	}

	output[0] = StateVariableFilter<T>::run(signalIn, filterStates[0], filterParams[0]) * 5;
	output[1] = StateVariableFilter<T>::run(StateVariableFilter<T>::run(signalIn, filterStates[1], filterParams[1]), filterStates[2], filterParams[2]) * 5;
	output[2] = StateVariableFilter<T>::run(StateVariableFilter<T>::run(signalIn, filterStates[3], filterParams[3]), filterStates[4], filterParams[4]) * 5;
	output[3] = StateVariableFilter<T>::run(signalIn, filterStates[5], filterParams[5]) * 5;

	for(int i=0; i<BANDS; i++) {		
		outputs[BAND_1_OUTPUT+i].setVoltage(output[i]);

		if(inputs[BAND_1_RETURN_INPUT+i].isConnected()) {
			out += inputs[BAND_1_RETURN_INPUT+i].getVoltage();
		} else {
			out += output[i];
		}
	}

	outputs[MIX_OUTPUT].setVoltage(out / 2.0); 
	
}


struct DamianLillardBandDisplay : TransparentWidget {
	DamianLillard *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	DamianLillardBandDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/01 Digit.ttf"));
	}

	void drawFrequency(const DrawArgs &args, Vec pos, float cutoffFrequency) {
		nvgFontSize(args.vg, 13);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);

		nvgFillColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0xff));
		char text[128];
		snprintf(text, sizeof(text), " % 4.0f", cutoffFrequency);
		nvgText(args.vg, pos.x + 8, pos.y, text, NULL);
	}

	void draw(const DrawArgs &args) override {
		if (!module)
			return;

		for(int i=0;i<FREQUENCIES;i++) {
			drawFrequency(args, Vec(i * 46.0, box.size.y - 75), module->freq[i]);
		}
	}
};

struct DamianLillardWidget : ModuleWidget {
	DamianLillardWidget(DamianLillard *module) {

		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DamianLillard.svg")));
		
		{
			DamianLillardBandDisplay *offsetDisplay = new DamianLillardBandDisplay();
			offsetDisplay->module = module;
			offsetDisplay->box.pos = Vec(15, 10);
			offsetDisplay->box.size = Vec(box.size.x, 140);
			addChild(offsetDisplay);
		}

		addParam(createParam<RoundBlackKnob>(Vec(15, 84), module, DamianLillard::FREQ_1_CUTOFF_PARAM));
		addParam(createParam<RoundBlackKnob>(Vec(66, 84), module, DamianLillard::FREQ_2_CUTOFF_PARAM));
		addParam(createParam<RoundBlackKnob>(Vec(117, 84), module, DamianLillard::FREQ_3_CUTOFF_PARAM));
		addParam(createParam<RoundSmallBlackKnob>(Vec(19, 146), module, DamianLillard::FREQ_1_CV_ATTENUVERTER_PARAM));
		addParam(createParam<RoundSmallBlackKnob>(Vec(70, 146), module, DamianLillard::FREQ_2_CV_ATTENUVERTER_PARAM));
		addParam(createParam<RoundSmallBlackKnob>(Vec(121, 146), module, DamianLillard::FREQ_3_CV_ATTENUVERTER_PARAM));



		addInput(createInput<PJ301MPort>(Vec(18, 117), module, DamianLillard::FREQ_1_CUTOFF_INPUT));
		addInput(createInput<PJ301MPort>(Vec(69, 117), module, DamianLillard::FREQ_2_CUTOFF_INPUT));
		addInput(createInput<PJ301MPort>(Vec(120, 117), module, DamianLillard::FREQ_3_CUTOFF_INPUT));

		addInput(createInput<PJ301MPort>(Vec(10, 317), module, DamianLillard::SIGNAL_IN));


		addInput(createInput<PJ301MPort>(Vec(10, 255), module, DamianLillard::BAND_1_RETURN_INPUT));
		addInput(createInput<PJ301MPort>(Vec(50, 255), module, DamianLillard::BAND_2_RETURN_INPUT));
		addInput(createInput<PJ301MPort>(Vec(90, 255), module, DamianLillard::BAND_3_RETURN_INPUT));
		addInput(createInput<PJ301MPort>(Vec(130, 255), module, DamianLillard::BAND_4_RETURN_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(10, 215), module, DamianLillard::BAND_1_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(50, 215), module, DamianLillard::BAND_2_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(90, 215), module, DamianLillard::BAND_3_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(130, 215), module, DamianLillard::BAND_4_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(90, 317), module, DamianLillard::MIX_OUTPUT));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH-12, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}
};


Model *modelDamianLillard = createModel<DamianLillard, DamianLillardWidget>("DamianLillard");
