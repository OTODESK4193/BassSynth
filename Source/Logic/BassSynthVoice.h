#pragma once
#include <JuceHeader.h>
#include "../DSP/WavetableOscillator.h"
#include "../DSP/SpectralMorphProcessor.h"
#include "../DSP/DualFilterEngine.h"
#include "../DSP/SineShaper.h"
#include "AdsrEnvelope.h"
#include "Lfo.h"
#include "Mseg.h"

struct VoiceParams {
    std::atomic<float>* pOscOn = nullptr;
    std::atomic<float>* pWave = nullptr;
    std::atomic<float>* pPos = nullptr;
    std::atomic<float>* pOscLevel = nullptr;
    std::atomic<float>* pOscPitch = nullptr;
    std::atomic<float>* pPDecayAmt = nullptr;
    std::atomic<float>* pPDecayTime = nullptr;
    std::atomic<float>* pFm = nullptr;
    std::atomic<float>* pFmWave = nullptr;
    std::atomic<float>* pMorphAMode = nullptr;
    std::atomic<float>* pMorphAAmt = nullptr;
    std::atomic<float>* pMorphAShift = nullptr;
    std::atomic<float>* pMorphBMode = nullptr;
    std::atomic<float>* pMorphBAmt = nullptr;
    std::atomic<float>* pMorphBShift = nullptr;
    std::atomic<float>* pMorphCMode = nullptr;
    std::atomic<float>* pMorphCAmt = nullptr;
    std::atomic<float>* pMorphCShift = nullptr;
    std::atomic<float>* pUni = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pDrift = nullptr;
    std::atomic<float>* pSubOn = nullptr;
    std::atomic<float>* pSubWave = nullptr;
    std::atomic<float>* pSubVol = nullptr;
    std::atomic<float>* pSubPitch = nullptr;

    std::atomic<float>* pFltAType = nullptr;
    std::atomic<float>* pFltACutoff = nullptr;
    std::atomic<float>* pFltAReso = nullptr;
    std::atomic<float>* pFltBType = nullptr;
    std::atomic<float>* pFltBCutoff = nullptr;
    std::atomic<float>* pFltBReso = nullptr;
    std::atomic<float>* pFltRouting = nullptr;
    std::atomic<float>* pFltMix = nullptr;
    std::atomic<float>* pFltAEnvAmt = nullptr;
    std::atomic<float>* pFltBEnvAmt = nullptr;

    std::atomic<float>* pAAtk = nullptr;
    std::atomic<float>* pADec = nullptr;
    std::atomic<float>* pASus = nullptr;
    std::atomic<float>* pARel = nullptr;
    std::atomic<float>* pFAtkA = nullptr;
    std::atomic<float>* pFDecA = nullptr;
    std::atomic<float>* pFSusA = nullptr;
    std::atomic<float>* pFRelA = nullptr;
    std::atomic<float>* pFAtkB = nullptr;
    std::atomic<float>* pFDecB = nullptr;
    std::atomic<float>* pFSusB = nullptr;
    std::atomic<float>* pFRelB = nullptr;

    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pShpAmt = nullptr;
    std::atomic<float>* pShpRate = nullptr;
    std::atomic<float>* pShpBit = nullptr;

    std::array<std::atomic<float>*, 3> pModOn, pModAtk, pModDec, pModSus, pModRel, pModAmt;
    std::array<std::atomic<float>*, 3> pModBipolar;
    std::array<std::atomic<float>*, 3> pLfoOn, pLfoWave, pLfoSync, pLfoRate, pLfoBeat, pLfoAmt, pLfoTrig;
    std::array<std::atomic<float>*, 3> pLfoUnipolar;
    std::array<std::atomic<float>*, 2> pMsegOn, pMsegSync, pMsegRate, pMsegBeat, pMsegAmt, pMsegTrig;
    std::array<std::atomic<float>*, 2> pMsegUnipolar;

    std::array<std::atomic<float>*, 10> pMatrixSrc;
    std::array<std::atomic<float>*, 10> pMatrixDest;
    std::array<std::atomic<float>*, 10> pMatrixAmt;
    std::atomic<float>* pColorMix = nullptr;
    std::atomic<float>* pGlide = nullptr;
};

class BassSynthVoice
{
public:
    BassSynthVoice() = default;

    void init(const VoiceParams& params) {
        p = params;
    }

    void prepare(double sr, int maxBlockSize) {
        sampleRate = std::max(1.0, sr);
        oscillator.prepare(sampleRate);
        spectralMorph.prepare(sampleRate, maxBlockSize);
        dualFilter.prepare(sampleRate, maxBlockSize);
        shaper.prepare(sampleRate);
        ampEnv.setSampleRate(sampleRate);
        filterEnvA.setSampleRate(sampleRate);
        filterEnvB.setSampleRate(sampleRate);
        for (int i = 0; i < 3; ++i) {
            modEnvs[i].setSampleRate(sampleRate);
            smoothedLfoRates[i].reset(sampleRate, 0.05);
        }
        for (int i = 0; i < 3; ++i) lfos[i].setSampleRate(sampleRate);
        for (int i = 0; i < 2; ++i) msegs[i].setSampleRate(sampleRate);

        tempEnvBuffer.setSize(24, maxBlockSize, true, true, true);
        tempSubBuffer.setSize(2, maxBlockSize, true, true, true);
        tempWavetableBuffer.setSize(2, maxBlockSize, true, true, true);

        smoothedWtLevel.reset(sampleRate, 0.005);
        smoothedWtPitch.reset(sampleRate, 0.005);
        smoothedPDecayAmt.reset(sampleRate, 0.005);
        smoothedPDecayTime.reset(sampleRate, 0.005);
        smoothedFltACutoff.reset(sampleRate, 0.01);
        smoothedFltAReso.reset(sampleRate, 0.01);
        smoothedFltBCutoff.reset(sampleRate, 0.01);
        smoothedFltBReso.reset(sampleRate, 0.01);
        smoothedFltMix.reset(sampleRate, 0.01);
        smoothedWtPos.reset(sampleRate, 0.01);
        smoothedFm.reset(sampleRate, 0.01);
        smoothedDrift.reset(sampleRate, 0.01);
        smoothedSubVol.reset(sampleRate, 0.005);
        smoothedMorphAAmt.reset(sampleRate, 0.01);
        smoothedMorphAShift.reset(sampleRate, 0.01);
        smoothedMorphBAmt.reset(sampleRate, 0.01);
        smoothedMorphBShift.reset(sampleRate, 0.01);
        smoothedMorphCAmt.reset(sampleRate, 0.01);
        smoothedMorphCShift.reset(sampleRate, 0.01);
        
        smoothedDrive.reset(sampleRate, 0.01);
        smoothedShpAmt.reset(sampleRate, 0.01);
        smoothedShpRate.reset(sampleRate, 0.01);
        smoothedShpBit.reset(sampleRate, 0.01);
    }

    void loadFactoryWavetable(int index) {
        oscillator.loadFactoryWavetable(index);
    }

    void loadCustomWavetable(const juce::File& file) {
        oscillator.loadCustomWavetableFile(file);
    }

    void noteOn(int noteNumber, float velocity, bool isLegato) {
        currentNoteNumber = noteNumber;
        currentVelocity = velocity;
        isActive = true;
        onTime = ++globalTimeCounter;

        targetFrequency = (float)juce::MidiMessage::getMidiNoteInHertz(noteNumber);
        if (!isLegato) {
            currentFrequency = targetFrequency;
            oscillator.resetPhase();
        }

        ampEnv.noteOn(isLegato);
        filterEnvA.noteOn(isLegato);
        filterEnvB.noteOn(isLegato);
        for (auto& env : modEnvs) env.noteOn(isLegato);
        for (auto& lfo : lfos) lfo.noteOn(isLegato);
        for (auto& ms : msegs) ms.noteOn(isLegato);
    }

    void noteOff(bool isLegato) {
        juce::ignoreUnused(isLegato);
        ampEnv.noteOff();
        filterEnvA.noteOff();
        filterEnvB.noteOff();
        for (auto& env : modEnvs) env.noteOff();
    }

    void updateMsegStates(const MsegState& state0, const MsegState& state1) {
        msegs[0].pushNewState(state0);
        msegs[1].pushNewState(state1);
    }

    bool getIsActive() const { return isActive; }
    int getNoteNumber() const { return currentNoteNumber; }
    uint32_t getOnTime() const { return onTime; }
    Mseg& getMsegEngine(int index) { return msegs[index]; }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, double bpm) {
        if (!isActive) return;

        int numSamples = outputBuffer.getNumSamples();

        float glideTimeMs = p.pGlide->load(std::memory_order_relaxed);
        float glideCoef = 0.0f;
        if (glideTimeMs > 0.001f) {
            glideCoef = std::exp(-std::log(1000.0f) / (glideTimeMs * 0.001f * (float)sampleRate));
        }

        // パラメータ値のスムージング更新とターゲット設定
        smoothedWtLevel.setTargetValue(p.pOscLevel->load(std::memory_order_relaxed));
        smoothedWtPitch.setTargetValue(p.pOscPitch->load(std::memory_order_relaxed));
        smoothedPDecayAmt.setTargetValue(p.pPDecayAmt->load(std::memory_order_relaxed));
        smoothedPDecayTime.setTargetValue(p.pPDecayTime->load(std::memory_order_relaxed));
        smoothedFltACutoff.setTargetValue(p.pFltACutoff->load(std::memory_order_relaxed));
        smoothedFltAReso.setTargetValue(p.pFltAReso->load(std::memory_order_relaxed));
        smoothedFltBCutoff.setTargetValue(p.pFltBCutoff->load(std::memory_order_relaxed));
        smoothedFltBReso.setTargetValue(p.pFltBReso->load(std::memory_order_relaxed));
        smoothedFltMix.setTargetValue(p.pFltMix->load(std::memory_order_relaxed));
        smoothedWtPos.setTargetValue(p.pPos->load(std::memory_order_relaxed));
        smoothedFm.setTargetValue(p.pFm->load(std::memory_order_relaxed));
        smoothedDrift.setTargetValue(p.pDrift->load(std::memory_order_relaxed));
        smoothedSubVol.setTargetValue(p.pSubVol->load(std::memory_order_relaxed));
        smoothedMorphAAmt.setTargetValue(p.pMorphAAmt->load(std::memory_order_relaxed));
        smoothedMorphAShift.setTargetValue(p.pMorphAShift->load(std::memory_order_relaxed));
        smoothedMorphBAmt.setTargetValue(p.pMorphBAmt->load(std::memory_order_relaxed));
        smoothedMorphBShift.setTargetValue(p.pMorphBShift->load(std::memory_order_relaxed));
        smoothedMorphCAmt.setTargetValue(p.pMorphCAmt->load(std::memory_order_relaxed));
        smoothedMorphCShift.setTargetValue(p.pMorphCShift->load(std::memory_order_relaxed));
        
        smoothedDrive.setTargetValue(p.pDrive->load(std::memory_order_relaxed));
        smoothedShpAmt.setTargetValue(p.pShpAmt->load(std::memory_order_relaxed));
        smoothedShpRate.setTargetValue(p.pShpRate->load(std::memory_order_relaxed));
        smoothedShpBit.setTargetValue(p.pShpBit->load(std::memory_order_relaxed));

        for (int i = 0; i < 3; ++i) {
            smoothedLfoRates[i].setTargetValue(p.pLfoRate[i]->load());
        }

        // オシレーターとフィルターの設定
        oscillator.setOscOn(p.pOscOn->load(std::memory_order_relaxed) > 0.5f);
        oscillator.setSubOn(p.pSubOn->load(std::memory_order_relaxed) > 0.5f);
        oscillator.setSubWaveform((int)p.pSubWave->load(std::memory_order_relaxed));
        oscillator.setSubPitchOffset(p.pSubPitch->load(std::memory_order_relaxed));
        oscillator.setUnisonCount((int)p.pUni->load(std::memory_order_relaxed));
        oscillator.setUnisonDetune(p.pDetune->load(std::memory_order_relaxed));
        oscillator.setFMWaveform((int)p.pFmWave->load(std::memory_order_relaxed));

        ampEnv.setParameters(p.pAAtk->load(std::memory_order_relaxed), p.pADec->load(std::memory_order_relaxed), p.pASus->load(std::memory_order_relaxed), p.pARel->load(std::memory_order_relaxed));
        filterEnvA.setParameters(p.pFAtkA->load(std::memory_order_relaxed), p.pFDecA->load(std::memory_order_relaxed), p.pFSusA->load(std::memory_order_relaxed), p.pFRelA->load(std::memory_order_relaxed));
        filterEnvB.setParameters(p.pFAtkB->load(std::memory_order_relaxed), p.pFDecB->load(std::memory_order_relaxed), p.pFSusB->load(std::memory_order_relaxed), p.pFRelB->load(std::memory_order_relaxed));

        for (int i = 0; i < 3; ++i) {
            modEnvs[i].setParameters(p.pModAtk[i]->load(), p.pModDec[i]->load(), p.pModSus[i]->load(), p.pModRel[i]->load());
        }
        for (int i = 0; i < 2; ++i) {
            msegs[i].setParameters(p.pMsegSync[i]->load() > 0.5f, p.pMsegRate[i]->load(), (int)p.pMsegBeat[i]->load(), p.pMsegAmt[i]->load(), (int)p.pMsegTrig[i]->load());
        }

        // バッファのリサイズ
        if (tempEnvBuffer.getNumSamples() < numSamples) {
            tempEnvBuffer.setSize(24, numSamples, true, true, true);
            tempSubBuffer.setSize(2, numSamples, true, true, true);
            tempWavetableBuffer.setSize(2, numSamples, true, true, true);
        }

        auto* wtL = tempWavetableBuffer.getWritePointer(0); auto* wtR = tempWavetableBuffer.getWritePointer(1);
        float stft_aA = 0.0f, stft_sA = 0.0f, stft_aB = 0.0f, stft_sB = 0.0f, stft_aC = 0.0f, stft_sC = 0.0f;

        float modAlpha = std::exp(-1.0f / static_cast<float>(sampleRate * 0.003));
        float destMods[23] = { 0.0f };

        // サンプル処理ループ
        for (int i = 0; i < numSamples; ++i) {
            for (int m = 0; m < 3; ++m) {
                float r = juce::jlimit(0.01f, 50.0f, smoothedLfoRates[m].getNextValue() + destMods[20 + m] * 25.0f);
                lfos[m].setParameters((int)p.pLfoWave[m]->load(), p.pLfoSync[m]->load() > 0.5f, r, (int)p.pLfoBeat[m]->load(), p.pLfoAmt[m]->load(), (int)p.pLfoTrig[m]->load());
            }

            // モジュレーション極性の適用
            float rawSources[9] = {
                0.0f,
                (p.pModOn[0]->load(std::memory_order_relaxed) > 0.5f ? (p.pModBipolar[0]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[0].getNextSample() * 2.0f - 1.0f : modEnvs[0].getNextSample()) : 0.0f),
                (p.pModOn[1]->load(std::memory_order_relaxed) > 0.5f ? (p.pModBipolar[1]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[1].getNextSample() * 2.0f - 1.0f : modEnvs[1].getNextSample()) : 0.0f),
                (p.pModOn[2]->load(std::memory_order_relaxed) > 0.5f ? (p.pModBipolar[2]->load(std::memory_order_relaxed) > 0.5f ? modEnvs[2].getNextSample() * 2.0f - 1.0f : modEnvs[2].getNextSample()) : 0.0f),
                (p.pLfoOn[0]->load(std::memory_order_relaxed) > 0.5f ? (p.pLfoUnipolar[0]->load(std::memory_order_relaxed) > 0.5f ? (lfos[0].getNextSample(bpm) + 1.0f) * 0.5f : lfos[0].getNextSample(bpm)) : 0.0f),
                (p.pLfoOn[1]->load(std::memory_order_relaxed) > 0.5f ? (p.pLfoUnipolar[1]->load(std::memory_order_relaxed) > 0.5f ? (lfos[1].getNextSample(bpm) + 1.0f) * 0.5f : lfos[1].getNextSample(bpm)) : 0.0f),
                (p.pLfoOn[2]->load(std::memory_order_relaxed) > 0.5f ? (p.pLfoUnipolar[2]->load(std::memory_order_relaxed) > 0.5f ? (lfos[2].getNextSample(bpm) + 1.0f) * 0.5f : lfos[2].getNextSample(bpm)) : 0.0f),
                (p.pMsegOn[0]->load(std::memory_order_relaxed) > 0.5f ? (p.pMsegUnipolar[0]->load(std::memory_order_relaxed) > 0.5f ? (msegs[0].getNextSample(bpm) + 1.0f) * 0.5f : msegs[0].getNextSample(bpm)) : 0.0f),
                (p.pMsegOn[1]->load(std::memory_order_relaxed) > 0.5f ? (p.pMsegUnipolar[1]->load(std::memory_order_relaxed) > 0.5f ? (msegs[1].getNextSample(bpm) + 1.0f) * 0.5f : msegs[1].getNextSample(bpm)) : 0.0f)
            };

            for (int s = 1; s < 9; ++s) {
                modSourceStates[s] = modAlpha * modSourceStates[s] + (1.0f - modAlpha) * rawSources[s];
            }

            std::fill(std::begin(destMods), std::end(destMods), 0.0f);
            for (int slot = 0; slot < 10; ++slot) {
                int srcIdx = (int)p.pMatrixSrc[slot]->load(std::memory_order_relaxed);
                int destIdx = (int)p.pMatrixDest[slot]->load(std::memory_order_relaxed);
                float amt = p.pMatrixAmt[slot]->load(std::memory_order_relaxed);

                if (srcIdx > 0 && srcIdx < 9 && destIdx > 0 && destIdx < 23) {
                    destMods[destIdx] += modSourceStates[srcIdx] * amt;
                }
            }

            float modPos = juce::jlimit(0.0f, 1.0f, smoothedWtPos.getNextValue() + destMods[1]);
            float modFm = juce::jlimit(0.0f, 3.0f, smoothedFm.getNextValue() + destMods[2] * 3.0f);
            float aA = juce::jlimit(-1.0f, 1.0f, smoothedMorphAAmt.getNextValue() + destMods[3]);
            float sA = juce::jlimit(-1.0f, 1.0f, smoothedMorphAShift.getNextValue() + destMods[4]);
            float aB = juce::jlimit(-1.0f, 1.0f, smoothedMorphBAmt.getNextValue() + destMods[5]);
            float sB = juce::jlimit(-1.0f, 1.0f, smoothedMorphBShift.getNextValue() + destMods[6]);
            float aC = juce::jlimit(-1.0f, 1.0f, smoothedMorphCAmt.getNextValue() + destMods[7]);
            float sC = juce::jlimit(-1.0f, 1.0f, smoothedMorphCShift.getNextValue() + destMods[8]);

            float modWtPitch = smoothedWtPitch.getNextValue() + destMods[14] * 24.0f;
            float modDrive = juce::jlimit(1.0f, 10.0f, smoothedDrive.getNextValue() + destMods[15] * 9.0f);
            float modShpAmt = juce::jlimit(0.0f, 1.0f, smoothedShpAmt.getNextValue() + destMods[16]);
            float modRate = juce::jlimit(1.0f, 20.0f, smoothedShpRate.getNextValue() + destMods[17] * 19.0f);
            float modBits = juce::jlimit(1.0f, 24.0f, smoothedShpBit.getNextValue() + destMods[18] * 23.0f);
            float modColorMix = juce::jlimit(0.0f, 1.0f, p.pColorMix->load() + destMods[19]);

            if (i == 0) { 
                stft_aA = aA; stft_sA = sA; stft_aB = aB; stft_sB = sB; stft_aC = aC; stft_sC = sC; 
                currentModeA = (int)p.pMorphAMode->load(std::memory_order_relaxed);
                currentModeB = (int)p.pMorphBMode->load(std::memory_order_relaxed);
                currentModeC = (int)p.pMorphCMode->load(std::memory_order_relaxed);
            }

            oscillator.setWavetablePosition(modPos);
            oscillator.setFMAmount(modFm);
            oscillator.setWavetablePitchOffset(modWtPitch);
            oscillator.setPitchDecay(smoothedPDecayAmt.getNextValue(), smoothedPDecayTime.getNextValue());
            oscillator.setDriftAmount(smoothedDrift.getNextValue());
            oscillator.setMorphA(currentModeA, aA, sA);
            oscillator.setMorphB(currentModeB, aB, sB);
            oscillator.setMorphC(currentModeC, aC, sC);
            oscillator.setWavetableLevel(smoothedWtLevel.getNextValue());
            oscillator.setSubVolume(smoothedSubVol.getNextValue());

            // ピッチとグライド
            if (glideCoef > 0.0f) {
                currentFrequency = targetFrequency + (currentFrequency - targetFrequency) * glideCoef;
            }
            else {
                currentFrequency = targetFrequency;
            }
            float cf = currentFrequency;
            if (cf < 1.0f) cf = 1.0f;
            oscillator.setFrequency(cf);

            float aVal = ampEnv.getNextSample();
            float fValA = filterEnvA.getNextSample();
            float fValB = filterEnvB.getNextSample();

            if (ampEnv.popJustReset()) oscillator.resetPhase();

            tempEnvBuffer.setSample(0, i, aVal);
            tempEnvBuffer.setSample(1, i, fValA);
            tempEnvBuffer.setSample(2, i, fValB);
            tempEnvBuffer.setSample(3, i, destMods[9]);
            tempEnvBuffer.setSample(4, i, destMods[10]);
            tempEnvBuffer.setSample(5, i, destMods[11]);
            tempEnvBuffer.setSample(6, i, destMods[12]);
            tempEnvBuffer.setSample(7, i, destMods[13]);

            tempEnvBuffer.setSample(8, i, modDrive);
            tempEnvBuffer.setSample(9, i, modShpAmt);
            tempEnvBuffer.setSample(10, i, modRate);
            tempEnvBuffer.setSample(11, i, modBits);
            tempEnvBuffer.setSample(12, i, modColorMix);

            float oL = 0.0f, oR = 0.0f, subL = 0.0f, subR = 0.0f;
            oscillator.getSampleStereo(oL, oR, subL, subR);
            wtL[i] = oL; wtR[i] = oR;
            tempSubBuffer.setSample(0, i, subL); tempSubBuffer.setSample(1, i, subR);
        }

        // CPU負荷対策: Morphが完全に0でないときのみリアルタイムFFTを実行
        bool isSpA = (currentModeA >= 8 && currentModeA <= 13) && (std::abs(stft_aA) > 0.001f);
        bool isSpB = (currentModeB >= 8 && currentModeB <= 13) && (std::abs(stft_aB) > 0.001f);
        bool isSpC = (currentModeC >= 8 && currentModeC <= 13) && (std::abs(stft_aC) > 0.001f);
        if (isSpA || isSpB || isSpC) {
            spectralMorph.process(tempWavetableBuffer, currentModeA, stft_aA, stft_sA, currentModeB, stft_aB, stft_sB, currentModeC, stft_aC, stft_sC);
        }

        // ボイス個別でウェーブシェーピング (Shaper) の処理
        for (int i = 0; i < numSamples; ++i) {
            float cd = tempEnvBuffer.getSample(8, i);
            float csa = tempEnvBuffer.getSample(9, i);
            float csr = tempEnvBuffer.getSample(10, i);
            float csb = tempEnvBuffer.getSample(11, i);
            float sL = wtL[i]; float sR = wtR[i];
            shaper.processStereo(sL, sR, cd, csa, csr, csb, sL, sR);
            wtL[i] = sL; wtR[i] = sR;
        }

        // ボイス個別でフィルター (dualFilter) の処理
        auto* outL = outputBuffer.getWritePointer(0);
        auto* outR = outputBuffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i) {
            float ccA = smoothedFltACutoff.getNextValue(), crA = smoothedFltAReso.getNextValue();
            float ccB = smoothedFltBCutoff.getNextValue(), crB = smoothedFltBReso.getNextValue();

            float aVal = tempEnvBuffer.getSample(0, i);
            float fValA = tempEnvBuffer.getSample(1, i);
            float fValB = tempEnvBuffer.getSample(2, i);
            float cutModA = tempEnvBuffer.getSample(3, i);
            float resModA = tempEnvBuffer.getSample(4, i);
            float cutModB = tempEnvBuffer.getSample(5, i);
            float resModB = tempEnvBuffer.getSample(6, i);

            float sL = wtL[i] + tempSubBuffer.getSample(0, i);
            float sR = wtR[i] + tempSubBuffer.getSample(1, i);

            float mcA = ccA * std::exp2(cutModA * 8.0f); mcA += (fValA * p.pFltAEnvAmt->load() * 10000.0f);
            float mcB = ccB * std::exp2(cutModB * 8.0f); mcB += (fValB * p.pFltBEnvAmt->load() * 10000.0f);

            dualFilter.setFilterA((int)p.pFltAType->load(), mcA, crA + resModA);
            dualFilter.setFilterB((int)p.pFltBType->load(), mcB, crB + resModB);
            dualFilter.setRouting((int)p.pFltRouting->load(), smoothedFltMix.getNextValue());

            dualFilter.processStereo(sL, sR);

            // ボイス出力を加算ミックス
            outL[i] += sL * aVal;
            outR[i] += sR * aVal;
        }

        // エンベロープが完全にリリースし切ったら非アクティブ化
        if (ampEnv.getNextSample() <= 0.0001f && ampEnv.popJustReset() == false) {
            isActive = false;
        }
    }

    void generateSingleCycle(std::array<float, 512>& buffer) {
        oscillator.generateSingleCycle(buffer);
    }

private:
    VoiceParams p;
    double sampleRate = 44100.0;
    bool isActive = false;
    int currentNoteNumber = -1;
    float currentVelocity = 0.0f;
    uint32_t onTime = 0;

    static inline std::atomic<uint32_t> globalTimeCounter{ 0 };

    float targetFrequency = 440.0f;
    float currentFrequency = 440.0f;

    WavetableOscillator oscillator;
    SpectralMorphProcessor spectralMorph;
    DualFilterEngine dualFilter;
    SineShaper shaper;
    
    AdsrEnvelope ampEnv, filterEnvA, filterEnvB;
    std::array<AdsrEnvelope, 3> modEnvs;
    std::array<Lfo, 3> lfos;
    std::array<Mseg, 2> msegs;

    juce::AudioBuffer<float> tempEnvBuffer;
    juce::AudioBuffer<float> tempSubBuffer;
    juce::AudioBuffer<float> tempWavetableBuffer;

    juce::SmoothedValue<float> smoothedWtLevel, smoothedWtPitch, smoothedPDecayAmt, smoothedPDecayTime;
    juce::SmoothedValue<float> smoothedFltACutoff, smoothedFltAReso, smoothedFltBCutoff, smoothedFltBReso, smoothedFltMix;
    juce::SmoothedValue<float> smoothedDrive, smoothedShpAmt, smoothedShpRate, smoothedShpBit;
    juce::SmoothedValue<float> smoothedWtPos, smoothedFm, smoothedDrift, smoothedSubVol;
    juce::SmoothedValue<float> smoothedMorphAAmt, smoothedMorphAShift, smoothedMorphBAmt, smoothedMorphBShift, smoothedMorphCAmt, smoothedMorphCShift;
    std::array<juce::SmoothedValue<float>, 3> smoothedLfoRates;

    std::array<float, 9> modSourceStates = { 0.0f };
    int currentModeA = 0, currentModeB = 0, currentModeC = 0;
};
