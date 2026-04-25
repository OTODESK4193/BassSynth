// ==============================================================================
// Source/UI/WavetableBrowser.h
// ==============================================================================
#pragma once
#include <JuceHeader.h>

class WavetableBrowser : public juce::Component
{
public:
    std::function<void(const juce::File&)> onCustomFileSelected;
    std::function<void(int)> onFactoryIndexSelected;
    std::function<void(const juce::StringArray&)> onUserFoldersChanged;
    std::function<void(const juce::StringArray&)> onFavoritesChanged;
    std::function<void()> onCloseRequested;

    WavetableBrowser(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
    {
        catModel.owner = this;
        subCatModel.owner = this;
        fileModel.owner = this;

        categories.add("All");
        categories.add("Factory");
        categories.add("Favorite");
        categories.add("User");

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

    void setFavorites(const juce::StringArray& favs) {
        favoritePaths = favs;
        if (categories[selectedCategoryIdx] == "Favorite") updateFiles();
    }

    void loadUserFolders(const juce::StringArray& folderPaths) {
        userFolders.clear();
        for (auto& path : folderPaths) {
            juce::File dir(path);
            if (dir.isDirectory()) addUserFolderInternal(dir, false);
        }
        if (categories[selectedCategoryIdx] == "User" || categories[selectedCategoryIdx] == "All") updateSubCategories();
    }

    void updateSubCategories()
    {
        subCategories.clear();
        juce::String selCat = categories[selectedCategoryIdx];

        if (selCat == "Factory") {
            subCategories.add("Basic");
        }
        else if (selCat == "Favorite") {
            subCategories.add("All");
        }
        else if (selCat == "User") {
            subCategories.add("Uncategorized");
            for (auto& uf : userFolders) {
                for (auto& w : uf.wavs) {
                    if (w.subCategory != "Uncategorized" && !subCategories.contains(w.subCategory)) {
                        subCategories.add(w.subCategory);
                    }
                }
            }
        }
        else if (selCat == "All") {
            subCategories.add("All");
            subCategories.add("Basic");
            for (auto& uf : userFolders) {
                for (auto& w : uf.wavs) {
                    if (!subCategories.contains(w.subCategory)) subCategories.add(w.subCategory);
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

        auto addFactory = [&]() {
            if (selSub == "All" || selSub == "Basic") {
                const char* names[] = { "Basic Morph", "PWM Sweep", "Sync Sweep", "Harmonic Build", "FM Sweep", "Saw Sync", "Vowel Sweep", "Sub Fade", "Metallic Sweep", "Noise Fade" };
                for (int i = 0; i < 10; ++i) currentList.add({ true, i, juce::File(), names[i] });
            }
            };

        auto addUser = [&]() {
            for (auto& uf : userFolders) {
                for (auto& w : uf.wavs) {
                    if (selSub == "All" || w.subCategory == selSub) {
                        currentList.add({ false, -1, w.file, w.file.getFileNameWithoutExtension() });
                    }
                }
            }
            };

        auto addFavorites = [&]() {
            for (auto& fav : favoritePaths) {
                if (fav.startsWith("Factory::")) {
                    int idx = fav.substring(9).getIntValue();
                    const char* names[] = { "Basic Morph", "PWM Sweep", "Sync Sweep", "Harmonic Build", "FM Sweep", "Saw Sync", "Vowel Sweep", "Sub Fade", "Metallic Sweep", "Noise Fade" };
                    if (idx >= 0 && idx < 10) currentList.add({ true, idx, juce::File(), names[idx] });
                }
                else {
                    juce::File f(fav);
                    if (f.existsAsFile()) currentList.add({ false, -1, f, f.getFileNameWithoutExtension() });
                }
            }
            };

        if (selCat == "Factory") addFactory();
        else if (selCat == "User") addUser();
        else if (selCat == "Favorite") addFavorites();
        else if (selCat == "All") { addFactory(); addUser(); }

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
            currentFactoryIndex = -1;
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
    juce::StringArray favoritePaths;
    int selectedCategoryIdx = 0;
    int selectedSubCategoryIdx = 0;
    int currentFactoryIndex = 0;
    juce::File currentCustomFile;

    struct ListItem {
        bool isFactory;
        int factoryIndex;
        juce::File file;
        juce::String name;
    };
    juce::Array<ListItem> currentList;

    struct UserWav {
        juce::File file;
        juce::String subCategory;
    };
    struct CustomFolder {
        juce::String name;
        juce::File folder;
        juce::Array<UserWav> wavs;
    };
    juce::Array<CustomFolder> userFolders;

    void openFolderChooser() {
        chooser = std::make_unique<juce::FileChooser>("Select Wavetable Folder", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
        chooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            if (fc.getResult().isDirectory()) addUserFolderInternal(fc.getResult(), true);
            });
    }

    void addUserFolderInternal(const juce::File& rootFolder, bool triggerCallback) {
        for (auto& uf : userFolders) {
            if (uf.folder == rootFolder) return;
        }

        CustomFolder cf;
        cf.folder = rootFolder;
        cf.name = rootFolder.getFileName();

        auto wavFiles = rootFolder.findChildFiles(juce::File::findFiles, true, "*.wav");
        for (auto& f : wavFiles) {
            juce::File parent = f.getParentDirectory();
            juce::String subCat = "Uncategorized";
            if (parent != rootFolder) subCat = parent.getFileName();
            cf.wavs.add({ f, subCat });
        }
        userFolders.add(cf);

        if (triggerCallback && onUserFoldersChanged) {
            juce::StringArray paths;
            for (auto& uf : userFolders) paths.add(uf.folder.getFullPathName());
            onUserFoldersChanged(paths);
        }

        if (categories[selectedCategoryIdx] == "User" || categories[selectedCategoryIdx] == "All") updateSubCategories();
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

            // ★ ★マークの描画
            juce::String favId = item.isFactory ? "Factory::" + juce::String(item.factoryIndex) : item.file.getFullPathName();
            bool isFav = owner->favoritePaths.contains(favId);

            g.setColour(isFav ? juce::Colour::fromString("FFFFD700") : juce::Colours::darkgrey);
            g.setFont(22.0f);
            g.drawText(isFav ? juce::String::fromUTF8("\xE2\x98\x85") : juce::String::fromUTF8("\xE2\x98\x86"), 8, 0, 25, h, juce::Justification::centred);

            g.setColour(isActive ? juce::Colours::white : juce::Colours::lightgrey);
            g.setFont(16.0f);
            g.drawText(item.name, 38, 0, w - 40, h, juce::Justification::centredLeft);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override {
            if (!owner) return;
            if (e.x < 35) { // ★ 星マークのクリック判定
                auto& item = owner->currentList.getReference(row);
                juce::String favId = item.isFactory ? "Factory::" + juce::String(item.factoryIndex) : item.file.getFullPathName();
                if (owner->favoritePaths.contains(favId)) owner->favoritePaths.removeString(favId);
                else owner->favoritePaths.add(favId);

                if (owner->onFavoritesChanged) owner->onFavoritesChanged(owner->favoritePaths);
                if (owner->categories[owner->selectedCategoryIdx] == "Favorite") owner->updateFiles();
                else owner->fileList.repaintRow(row);
            }
            else {
                owner->applySelection(row);
            }
        }
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override {
            if (!owner) return;
            owner->applySelection(row);
            if (owner->onCloseRequested) owner->onCloseRequested();
        }
    } fileModel;
};