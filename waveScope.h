//
//  waveScope.h
//  MIRABCN_Generator
//
//  Created by Eduard Frigola on 10/01/2017.
//
//

#ifndef waveScope_h
#define waveScope_h

#include "ofxOceanodeNodeModelExternalWindow.h"

template<typename T>
class multiDynamicParameters{
public:
    multiDynamicParameters(){};
    ~multiDynamicParameters(){};
    
    void setup(ofParameterGroup* _group, ofParameter<T> _baseParameter){
        group = _group;
        baseParameter.set(_baseParameter.getName(), _baseParameter.get());
        parameterVector[0] = baseParameter;
        parameterVector[0].setName(baseParameter.getName() + " 0");
        group->add(parameterVector[0]);
        ifNewCreatedChecker[0] = false;
        listeners[0] = parameterVector[0].newListener([&](T &val){
            inputListener(0);
        });
    }
    
    map<int, ofParameter<T>> &getParameters(){return parameterVector;};
    
    ofEvent<void> parameterGroupChanged;
    
    void saveParameterArrange(ofJson &json){
        vector<int> indexs;
        for(auto param : parameterVector) indexs.push_back(param.first);
        json["MultiPresetArrange"] = indexs;
    }
    
    void loadParameterArrange(ofJson &json){
        if(json.count("MultiPresetArrange") == 1){
            vector<int> indexs = json["MultiPresetArrange"];
            for(auto param : parameterVector){
                group->remove(param.second.getEscapedName());
                listeners.erase(param.first);
                ifNewCreatedChecker.erase(param.first);
            }
            parameterVector.clear();
            for(int i : indexs){
                parameterVector[i] = ofParameter<T>();
                parameterVector[i].set(baseParameter.getName() + " " + ofToString(i), baseParameter);
                ifNewCreatedChecker[i] = false;
                group->add(parameterVector[i]);
                listeners[i] = parameterVector[i].newListener([&, i](T &val){
                    inputListener(i);
                });
                ifNewCreatedChecker[i] = true;
            }
            ofNotifyEvent(parameterGroupChanged);
        }
    }
    
private:
    
    void inputListener(int index){
        if(parameterVector[index].get() != nullptr && !ifNewCreatedChecker[index]){
            int newCreatedIndex = -1;
            for(int i = 0 ; newCreatedIndex == -1 ; i++){
                if(parameterVector.count(i) == 0){
                    newCreatedIndex = i;
                }
            }
            parameterVector[newCreatedIndex] = ofParameter<T>();
            parameterVector[newCreatedIndex].set(baseParameter.getName() + " " + ofToString(newCreatedIndex), baseParameter);
            ifNewCreatedChecker[newCreatedIndex] = false;
            group->add(parameterVector[newCreatedIndex]);
            listeners[newCreatedIndex] = parameterVector[newCreatedIndex].newListener([&, newCreatedIndex](T &val){
                inputListener(newCreatedIndex);
            });
            ifNewCreatedChecker[index] = true;
            ofNotifyEvent(parameterGroupChanged);
        }
        else if(parameterVector[index].get() == nullptr){
            int removeIndex = -1;
            for(auto param : parameterVector){
                if(param.second == nullptr && param.first > removeIndex){
                    removeIndex = param.first;
                }
            }
            group->remove(parameterVector[removeIndex].getEscapedName());
            listeners.erase(removeIndex);
            parameterVector.erase(removeIndex);
            ifNewCreatedChecker.erase(removeIndex);
            if(index != removeIndex){
                ifNewCreatedChecker[index] = false;
            }
            ofNotifyEvent(parameterGroupChanged);
        }
    }
    
    ofParameterGroup* group;
    ofParameter<T> baseParameter;
    map<int, ofParameter<T>> parameterVector;
    map<int, bool> ifNewCreatedChecker;
    
    map<int, ofEventListener> listeners;
};

class waveScope : public ofxOceanodeNodeModelExternalWindow{
public:
    waveScope();
    ~waveScope(){};
    
    virtual void presetSave(ofJson &json) override{
        texturesInput.saveParameterArrange(json);
    };
    
    virtual void presetRecallBeforeSettingParameters(ofJson &json) override{
        texturesInput.loadParameterArrange(json);
    };

private:
    void drawInExternalWindow(ofEventArgs &e) override;
    
    ofEventListener listener;
    
    multiDynamicParameters<ofTexture*>  texturesInput;
    
    int contentWidthOffset;
    int mousePressInititalX;
    bool isInMovableRegion;
    bool hasColor;
};

#endif /* waveScope_h */
