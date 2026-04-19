// ==============================================================================
// Source/UI/WavetableBrowser.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "../Generated/WavetableData_Generated.h"

class WavetableBrowser : public juce::Component
{
public:
    WavetableBrowser(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
    {
        catModel.owner = this;
        subCatModel.owner = this;
        fileModel.owner = this;

        categories.add("All");
        // 【NEW】「Basic」カテゴリーを先頭に追加
        categories.add("Basic");

        for (int i = 0; i < EmbeddedWavetables::numTables; ++i) {
            juce::StringArray tags;
            tags.addTokens(EmbeddedWavetables::allTags[i], "|", "");
            if (tags.size() > 0 && !categories.contains(tags[0])) {
                categories.add(tags[0]);
            }
        }

        catList.setModel(&catModel);
        subCatList.setModel(&subCatModel);
        fileList.setModel(&fileModel);

        catList.setRowHeight(40);
        subCatList.setRowHeight(40);
        fileList.setRowHeight(40);

        catList.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromString("FA121212"));
        subCatList.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromString("FA1A1A1A"));
        fileList.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromString("FA222222"));

        addAndMakeVisible(catList);
        addAndMakeVisible(subCatList);
        addAndMakeVisible(fileList);

        currentWavetableIndex = (int)apvts.getRawParameterValue("osc_wave")->load();
        updateSubCategories();
    }

    void updateSubCategories()
    {
        subCategories.clear();
        subCategories.add("All");
        juce::String selCat = categories[selectedCategoryIdx];

        // 【NEW】Basicカテゴリー選択時はShapesサブカテゴリーを追加
        if (selCat == "All" || selCat == "Basic") {
            subCategories.add("Shapes");
        }

        for (int i = 0; i < EmbeddedWavetables::numTables; ++i) {
            juce::StringArray tags;
            tags.addTokens(EmbeddedWavetables::allTags[i], "|", "");
            if (selCat == "All" || (tags.size() > 0 && tags[0] == selCat)) {
                for (int j = 1; j < tags.size(); ++j) {
                    if (!subCategories.contains(tags[j])) subCategories.add(tags[j]);
                }
            }
        }
        selectedSubCategoryIdx = 0;
        subCatList.updateContent();
        updateFiles();
    }

    void updateFiles()
    {
        filteredIndices.clear();
        juce::String selCat = categories[selectedCategoryIdx];
        juce::String selSub = (selectedSubCategoryIdx >= 0 && selectedSubCategoryIdx < subCategories.size())
            ? subCategories[selectedSubCategoryIdx] : "All";

        // 【NEW】条件に合致すれば -1 (Basic Shapes) をリストに追加
        if ((selCat == "All" || selCat == "Basic") && (selSub == "All" || selSub == "Shapes")) {
            filteredIndices.add(-1);
        }

        for (int i = 0; i < EmbeddedWavetables::numTables; ++i) {
            juce::StringArray tags;
            tags.addTokens(EmbeddedWavetables::allTags[i], "|", "");

            bool matchCat = (selCat == "All") || (tags.size() > 0 && tags[0] == selCat);
            bool matchSub = (selSub == "All");
            if (!matchSub) {
                for (int j = 1; j < tags.size(); ++j) {
                    if (tags[j] == selSub) { matchSub = true; break; }
                }
            }

            if (matchCat && matchSub) filteredIndices.add(i);
        }
        fileList.updateContent();
    }

    void applySelection(int dataIdx) {
        currentWavetableIndex = dataIdx;
        if (auto* param = apvts.getParameter("osc_wave"))
            param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1((float)dataIdx));
    }

    void selectNext() {
        if (filteredIndices.isEmpty()) return;
        int currentListIdx = -1;
        for (int i = 0; i < filteredIndices.size(); ++i) {
            if (filteredIndices[i] == currentWavetableIndex) {
                currentListIdx = i; break;
            }
        }
        int nextIdx = (currentListIdx + 1) % filteredIndices.size();
        applySelection(filteredIndices[nextIdx]);
        fileList.selectRow(nextIdx);
        fileList.scrollToEnsureRowIsOnscreen(nextIdx);
    }

    void selectPrev() {
        if (filteredIndices.isEmpty()) return;
        int currentListIdx = -1;
        for (int i = 0; i < filteredIndices.size(); ++i) {
            if (filteredIndices[i] == currentWavetableIndex) {
                currentListIdx = i; break;
            }
        }
        int prevIdx = (currentListIdx - 1 + filteredIndices.size()) % filteredIndices.size();
        applySelection(filteredIndices[prevIdx]);
        fileList.selectRow(prevIdx);
        fileList.scrollToEnsureRowIsOnscreen(prevIdx);
    }

    void selectRandom() {
        if (filteredIndices.isEmpty()) return;
        int rndIdx = juce::Random::getSystemRandom().nextInt(filteredIndices.size());
        applySelection(filteredIndices[rndIdx]);
        fileList.selectRow(rndIdx);
        fileList.scrollToEnsureRowIsOnscreen(rndIdx);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour::fromString("FA111111"));
        g.setColour(juce::Colour::fromString("FF555555"));
        g.drawRect(getLocalBounds(), 2);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(2);
        int w = area.getWidth() / 3;
        catList.setBounds(area.removeFromLeft(w));
        subCatList.setBounds(area.removeFromLeft(w));
        fileList.setBounds(area);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::ListBox catList{ "Cat", nullptr }, subCatList{ "Sub", nullptr }, fileList{ "File", nullptr };

    juce::StringArray categories, subCategories;
    juce::Array<int> filteredIndices;
    int selectedCategoryIdx = 0;
    int selectedSubCategoryIdx = 0;
    int currentWavetableIndex = -1;

    struct CatModel : public juce::ListBoxModel {
        WavetableBrowser* owner = nullptr;
        int getNumRows() override { return owner ? owner->categories.size() : 0; }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            if (!owner) return;
            bool isActive = (row == owner->selectedCategoryIdx);
            if (isActive) g.fillAll(juce::Colour::fromString("FF4A4A4A"));
            g.setColour(isActive ? juce::Colours::white : juce::Colours::grey);
            g.setFont(18.0f);
            g.drawText(owner->categories[row], 15, 0, w - 30, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override {
            if (!owner) return;
            owner->selectedCategoryIdx = row;
            owner->updateSubCategories();
        }
    } catModel;

    struct SubCatModel : public juce::ListBoxModel {
        WavetableBrowser* owner = nullptr;
        int getNumRows() override { return owner ? owner->subCategories.size() : 0; }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            if (!owner) return;
            bool isActive = (row == owner->selectedSubCategoryIdx);
            if (isActive) g.fillAll(juce::Colour::fromString("FF5A5A5A"));
            g.setColour(isActive ? juce::Colours::white : juce::Colours::grey);
            g.setFont(18.0f);
            g.drawText(owner->subCategories[row], 15, 0, w - 30, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override {
            if (!owner) return;
            owner->selectedSubCategoryIdx = row;
            owner->updateFiles();
        }
    } subCatModel;

    struct FileModel : public juce::ListBoxModel {
        WavetableBrowser* owner = nullptr;
        int getNumRows() override { return owner ? owner->filteredIndices.size() : 0; }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            if (!owner) return;
            bool isActive = (owner->filteredIndices[row] == owner->currentWavetableIndex);
            if (isActive) g.fillAll(juce::Colour::fromString("FF6A6A6A"));
            g.setColour(isActive ? juce::Colours::white : juce::Colours::lightgrey);
            g.setFont(16.0f);
            int dataIdx = owner->filteredIndices[row];

            // 【NEW】 -1 の場合は名前を「Basic Shapes」として描画
            juce::String displayName = (dataIdx == -1) ? "Basic Shapes" : EmbeddedWavetables::allNames[dataIdx];
            g.drawText(displayName, 15, 0, w - 30, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override {
            if (!owner) return;
            int dataIdx = owner->filteredIndices[row];
            owner->applySelection(dataIdx);
            owner->fileList.repaint();
        }
    } fileModel;
};