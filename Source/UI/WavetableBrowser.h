// ==============================================================================
// Source/UI/WavetableBrowser.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>
#include "../Generated/WavetableData_Generated.h"

class WavetableBrowser : public juce::Component
{
public:
    std::function<void(const juce::File&)> onCustomFileSelected;
    std::function<void(int)> onFactoryIndexSelected;
    std::function<void(const juce::StringArray&)> onUserFoldersChanged;

    // ★ 追加: ダブルクリック時にブラウザを閉じるためのコールバック
    std::function<void()> onCloseRequested;

    WavetableBrowser(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
    {
        catModel.owner = this;
        subCatModel.owner = this;
        fileModel.owner = this;

        categories.add("Basic");
        categories.add("User");
        categories.add("All");

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

        addAndMakeVisible(addFolderBtn);
        addFolderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour::fromString("FF2A2A2A"));
        addFolderBtn.onClick = [this] { openFolderChooser(); };

        currentFactoryIndex = (int)apvts.getRawParameterValue("osc_wave")->load();
        updateSubCategories();
    }

    void loadUserFolders(const juce::StringArray& folderPaths) {
        userFolders.clear();
        for (auto& path : folderPaths) {
            juce::File dir(path);
            if (dir.isDirectory()) {
                addUserFolderInternal(dir, false);
            }
        }
        if (categories[selectedCategoryIdx] == "User") updateSubCategories();
    }

    void updateSubCategories()
    {
        subCategories.clear();
        juce::String selCat = categories[selectedCategoryIdx];

        if (selCat == "User") {
            for (auto& uf : userFolders) subCategories.add(uf.name);
        }
        else {
            subCategories.add("All");
            if (selCat == "All" || selCat == "Basic") subCategories.add("Shapes");

            for (int i = 0; i < EmbeddedWavetables::numTables; ++i) {
                juce::StringArray tags;
                tags.addTokens(EmbeddedWavetables::allTags[i], "|", "");
                if (selCat == "All" || (tags.size() > 0 && tags[0] == selCat)) {
                    for (int j = 1; j < tags.size(); ++j) {
                        if (!subCategories.contains(tags[j])) subCategories.add(tags[j]);
                    }
                }
            }
        }
        selectedSubCategoryIdx = 0;
        subCatList.updateContent();
        updateFiles();
    }

    void updateFiles()
    {
        currentList.clear();
        juce::String selCat = categories[selectedCategoryIdx];
        juce::String selSub = (selectedSubCategoryIdx >= 0 && selectedSubCategoryIdx < subCategories.size())
            ? subCategories[selectedSubCategoryIdx] : "";

        if (selCat == "User") {
            for (auto& uf : userFolders) {
                if (uf.name == selSub) {
                    for (auto& f : uf.files) {
                        currentList.add({ false, -1, f, f.getFileNameWithoutExtension() });
                    }
                    break;
                }
            }
        }
        else {
            if ((selCat == "All" || selCat == "Basic") && (selSub == "All" || selSub == "Shapes" || selSub.isEmpty())) {
                currentList.add({ true, -1, juce::File(), "Basic Shapes" });
            }

            for (int i = 0; i < EmbeddedWavetables::numTables; ++i) {
                juce::StringArray tags;
                tags.addTokens(EmbeddedWavetables::allTags[i], "|", "");

                bool matchCat = (selCat == "All") || (tags.size() > 0 && tags[0] == selCat);
                bool matchSub = (selSub == "All" || selSub.isEmpty());
                if (!matchSub) {
                    for (int j = 1; j < tags.size(); ++j) {
                        if (tags[j] == selSub) { matchSub = true; break; }
                    }
                }

                if (matchCat && matchSub) {
                    currentList.add({ true, i, juce::File(), EmbeddedWavetables::allNames[i] });
                }
            }
        }
        fileList.updateContent();
    }

    void applySelection(int row) {
        if (row < 0 || row >= currentList.size()) return;
        auto& item = currentList.getReference(row);

        if (item.isFactory) {
            currentFactoryIndex = item.factoryIndex;
            currentCustomFile = juce::File();
            if (auto* param = apvts.getParameter("osc_wave"))
                param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1((float)item.factoryIndex));

            if (onFactoryIndexSelected) onFactoryIndexSelected(item.factoryIndex);
        }
        else {
            currentFactoryIndex = -2;
            currentCustomFile = item.file;
            if (onCustomFileSelected) onCustomFileSelected(item.file);
        }
        fileList.repaint();
    }

    void selectNext() { moveSelection(1); }
    void selectPrev() { moveSelection(-1); }
    void selectRandom() {
        if (currentList.isEmpty()) return;
        int rndIdx = juce::Random::getSystemRandom().nextInt(currentList.size());
        applySelection(rndIdx);
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

        auto catArea = area.removeFromLeft(w);
        auto subArea = area.removeFromLeft(w);

        addFolderBtn.setBounds(subArea.removeFromBottom(30).reduced(2));

        catList.setBounds(catArea);
        subCatList.setBounds(subArea);
        fileList.setBounds(area);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::ListBox catList{ "Cat", nullptr }, subCatList{ "Sub", nullptr }, fileList{ "File", nullptr };
    juce::TextButton addFolderBtn{ "+ Add User Folder" };
    std::unique_ptr<juce::FileChooser> chooser;

    juce::StringArray categories, subCategories;
    int selectedCategoryIdx = 0;
    int selectedSubCategoryIdx = 0;
    int currentFactoryIndex = -1;
    juce::File currentCustomFile;

    struct ListItem {
        bool isFactory;
        int factoryIndex;
        juce::File file;
        juce::String name;
    };
    juce::Array<ListItem> currentList;

    struct CustomFolder {
        juce::String name;
        juce::File folder;
        juce::Array<juce::File> files;
    };
    juce::Array<CustomFolder> userFolders;

    void openFolderChooser() {
        chooser = std::make_unique<juce::FileChooser>("Select Wavetable Folder", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
        chooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            if (fc.getResult().isDirectory()) addUserFolderInternal(fc.getResult(), true);
            });
    }

    void addUserFolderInternal(const juce::File& folder, bool triggerCallback) {
        for (auto& uf : userFolders) {
            if (uf.folder == folder) return;
        }

        CustomFolder cf;
        cf.folder = folder;
        cf.name = folder.getFileName();
        auto files = folder.findChildFiles(juce::File::findFiles, false, "*.wav");
        for (auto& f : files) cf.files.add(f);
        userFolders.add(cf);

        if (triggerCallback && onUserFoldersChanged) {
            juce::StringArray paths;
            for (auto& uf : userFolders) paths.add(uf.folder.getFullPathName());
            onUserFoldersChanged(paths);
        }

        if (categories[selectedCategoryIdx] == "User") updateSubCategories();
    }

    void moveSelection(int delta) {
        if (currentList.isEmpty()) return;
        int currentListIdx = 0;
        for (int i = 0; i < currentList.size(); ++i) {
            if (currentList[i].isFactory && currentList[i].factoryIndex == currentFactoryIndex) { currentListIdx = i; break; }
            if (!currentList[i].isFactory && currentList[i].file == currentCustomFile) { currentListIdx = i; break; }
        }
        int nextIdx = (currentListIdx + delta + currentList.size()) % currentList.size();
        applySelection(nextIdx);
        fileList.selectRow(nextIdx);
        fileList.scrollToEnsureRowIsOnscreen(nextIdx);
    }

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
        int getNumRows() override { return owner ? owner->currentList.size() : 0; }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override {
            if (!owner) return;
            auto& item = owner->currentList.getReference(row);

            bool isActive = false;
            if (item.isFactory && item.factoryIndex == owner->currentFactoryIndex) isActive = true;
            if (!item.isFactory && item.file == owner->currentCustomFile) isActive = true;

            if (isActive) g.fillAll(juce::Colour::fromString("FF6A6A6A"));
            g.setColour(isActive ? juce::Colours::white : juce::Colours::lightgrey);
            g.setFont(16.0f);
            g.drawText(item.name, 15, 0, w - 30, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override {
            if (!owner) return;
            owner->applySelection(row);
        }
        // ★ 追加: ダブルクリック時の処理
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override {
            if (!owner) return;
            owner->applySelection(row);
            if (owner->onCloseRequested) owner->onCloseRequested();
        }
    } fileModel;
};