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
    multiDynamicParameters(ofParameterGroup& _group, ofParameter<T> _baseParameter): group(_group){
        baseParameter.set(_baseParameter.getName(), _baseParameter.get());
        parameterVector[0] = baseParameter;
        parameterVector[0].setName(baseParameter.getName() + " 0");
        auto oceaParam = make_shared<ofxOceanodeParameter<T>>();
        oceaParam->bindParameter(parameterVector[0]);
        group.add(*oceaParam);
        ifNewCreatedChecker[0] = false;
        listeners[0] = parameterVector[0].newListener([&](T &val){
            inputListener(0);
        });
    };
    ~multiDynamicParameters(){};
    
//    void setup(shared_ptr<ofParameterGroup> _group, ofParameter<T> _baseParameter){
//        group = _group;
//        baseParameter.set(_baseParameter.getName(), _baseParameter.get());
//        parameterVector[0] = baseParameter;
//        parameterVector[0].setName(baseParameter.getName() + " 0");
//        group->add(parameterVector[0]);
//        ifNewCreatedChecker[0] = false;
//        listeners[0] = parameterVector[0].newListener([&](T &val){
//            inputListener(0);
//        });
//    }
    
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
                auto result = std::find(indexs.begin(), indexs.end(), param.first);
                if(result == indexs.end()){
                    group.remove(param.second.getEscapedName());
                    listeners.erase(param.first);
                    ifNewCreatedChecker.erase(param.first);
                }
                else{
                    ifNewCreatedChecker[param.first] = true;
                }
            }
            for(int i : indexs){
                if(parameterVector.count(i) == 0){
                    parameterVector[i] = ofParameter<T>();
                    parameterVector[i].set(baseParameter.getName() + " " + ofToString(i), baseParameter);
                    lastValueVector[i] = baseParameter;
                    auto oceaParam = make_shared<ofxOceanodeParameter<T>>();
                    oceaParam->bindParameter(parameterVector[i]);
                    group.add(*oceaParam);
                    listeners[i] = parameterVector[i].newListener([&, i](T &val){
                        inputListener(i);
                    });
                    ifNewCreatedChecker[i] = true;
                }
            }
            ifNewCreatedChecker[indexs.back()] = false;
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
            auto oceaParam = make_shared<ofxOceanodeParameter<T>>();
            oceaParam->bindParameter(parameterVector[newCreatedIndex]);
            group.add(*oceaParam);
            listeners[newCreatedIndex] = parameterVector[newCreatedIndex].newListener([&, newCreatedIndex](T &val){
                inputListener(newCreatedIndex);
            });
            ifNewCreatedChecker[index] = true;
            ofNotifyEvent(parameterGroupChanged);
        }
        else if(parameterVector[index].get() == nullptr && lastValueVector[index] != nullptr){
            int removeIndex = -1;
            for(auto param : parameterVector){
                if(param.second == nullptr && param.first > removeIndex){
                    removeIndex = param.first;
                }
            }
            group.remove(parameterVector[removeIndex].getEscapedName());
            listeners.erase(removeIndex);
            parameterVector.erase(removeIndex);
            ifNewCreatedChecker.erase(removeIndex);
            if(index != removeIndex){
                ifNewCreatedChecker[index] = false;
            }
            ofNotifyEvent(parameterGroupChanged);
        }
        lastValueVector[index] = parameterVector[index].get();
    }
    
    ofParameterGroup& group;
    ofParameter<T> baseParameter;
    map<int, ofParameter<T>> parameterVector;
    map<int, T> lastValueVector;
    map<int, bool> ifNewCreatedChecker;
    
    map<int, ofEventListener> listeners;
};

class waveScope : public ofxOceanodeNodeModelExternalWindow{
public:
    waveScope();
    ~waveScope();
    
    virtual void presetSave(ofJson &json) override{
        texturesInput->saveParameterArrange(json);
    };
    
//    virtual void presetRecallBeforeSettingParameters(ofJson &json) override{
//        texturesInput.loadParameterArrange(json);
//    };
    
    void loadBeforeConnections(ofJson &json) override{
        texturesInput->loadParameterArrange(json);
    }

private:
    void drawInExternalWindow(ofEventArgs &e) override;
    
    ofEventListener listener;
    
    std::unique_ptr<multiDynamicParameters<ofTexture*>>  texturesInput;
    
    int contentWidthOffset;
    int mousePressInititalX;
    bool isInMovableRegion;
    bool hasColor;
    
    ofRectangle storedShape;
};

#endif /* waveScope_h */
