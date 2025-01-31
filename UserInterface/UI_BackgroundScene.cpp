#include "stdafx.h"

#include "console.h"

#include "Serialization\Serialization.h"
#include "Serialization\ResourceSelector.h"
#include "XTL\Rect.h"

#include "Render\src\Scene.h"
#include "Render\src\cCamera.h"
#include "Render\3dx\Node3dx.h"
#include "Render\src\VisGeneric.h"

#include "Environment\Environment.h"
#include "Game\Universe.h"
#include "Game\Player.h"
#include "CameraManager.h"

#include "UI_Render.h"
#include "UI_BackgroundScene.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_BackgroundAnimation, PlayMode, "UI_BackgroundAnimation::PlayMode")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_STARTUP, "��� ���������")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_PERMANENT, "���������")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_HOVER_STARTUP, "��������� ��� ��������� ����")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_HOVER_PERMANENT, "��������� ��� ��������� ����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_BackgroundAnimation, PlayMode)

const float UI_BackgroundScene::scale2focus = 0.0025f;

// ------------------------------- UI_BackgroundModelSetup

UI_BackgroundModelSetup::UI_BackgroundModelSetup() :
	skinColor_(1.0f, 1.0f, 1.0f, 1.0f)
{
	useOwnColor_ = true;
	useEmblem_ = true;
	scale_ = 1.f;
}

void UI_BackgroundModelSetup::serialize(Archive& ar)
{
	ar.serialize(ModelSelector(modelName_, ModelSelector::DEFAULT_OPTIONS), "fileName", "������");

	ar.serialize(scale_, "scale", "������ ������");

	ar.serialize(useOwnColor_, "useOwnColor", "����������� ���� �������");
	if(useOwnColor_)
		ar.serialize(skinColor_, "skinColor", "����");

	ar.serialize(useEmblem_, "useEmblem", "������������ ������� �������");
 
	if(ar.isInput() && isUnderEditor())
		updateComboLists();
}

void UI_BackgroundModelSetup::updateComboLists()
{
	groupComboList_ = "";
	chainComboList_ = "";
	nodeComboList_ = "";

	if(modelName_.empty())
		return;

	cScene* scene = gb_VisGeneric->CreateScene();
	cObject3dx* model = scene->CreateObject3dx(modelName_.c_str());
	if(model){
		string comboList;
		int number = model->GetAnimationGroupNumber();
		int i;
		for(i = 0; i < number; i++){
			if(!comboList.empty())
				comboList += "|";
			comboList += model->GetAnimationGroupName(i);
		}
		groupComboList_ = comboList;

		comboList = "";
		number = model->GetChainNumber();
		for(i = 0; i < number; i++){
			if(!comboList.empty())
				comboList += "|";
			comboList += model->GetChain(i)->name;
		}
		chainComboList_ = comboList;

		comboList = "";
		number = model->GetNodeNumber();
		for(int i = 0; i < number; i++){
			comboList += "|";
			comboList += model->GetNodeName(i);
		}
		nodeComboList_ = comboList;

		model->Release();
	}

	scene->Release();
}

void UI_BackgroundModelSetup::preLoad(cScene* scene, const Player* player) const
{
	xassert(scene);
	
	cObject3dx* model = scene->CreateObject3dx(modelName(), NULL);
	xassert(model);

	model->DisableDetailLevel();
	
	if(player){
		Color4f color(ownSkinColor() ? skinColor() : player->unitColor());
		model->SetSkinColor(color, useEmblem() ? player->unitSign() : 0);
	}
	else
		model->SetSkinColor(skinColor(), 0);

	RELEASE(model);
}

// ------------------------------- UI_BackgroundModel

UI_BackgroundModel::UI_BackgroundModel() : model_(0)
{

}

void UI_BackgroundModel::load(cScene* scene, const UI_BackgroundModelSetup& setup, const Player* player)
{
	MTG();
	release();

#ifdef _DEBUG
	kdWarning("UI Background", (string("Model load ") + setup.modelName()).c_str());
#endif

	if(setup.isEmpty())
		return;

	model_ = scene->CreateObject3dx(setup.modelName(), NULL);
	model_->DisableDetailLevel();
	if(player){
		Color4f color(setup.ownSkinColor() ? setup.skinColor() : player->unitColor());
		model_->SetSkinColor(color, setup.useEmblem() ? player->unitSign() : 0);
	}
	else
		model_->SetSkinColor(setup.skinColor(), 0);

}

void UI_BackgroundModel::release()
{
	MTG();

#ifdef _DEBUG
	if(model_)
		kdWarning("UI Background", "Model release");
#endif

	animations_.clear();

	Effects::iterator eit;
	FOR_EACH(effects_, eit)
		eit->effectStop();

	effects_.clear();

	if(model_){
		model_->Release();
		model_ = 0;
	}
}

void UI_BackgroundModel::setPosition(const MatXf& pos)
{
	MTG();
	if(model_)
		model_->SetPosition(pos);
}

bool UI_BackgroundModel::isPlaying() const
{
	MTG();
	return !animations_.empty();
}

bool UI_BackgroundModel::isPlaying(UI_BackgroundAnimation::PlayMode mode) const
{
	MTG();
	for(UI_BackgroundAnimationControllers::const_iterator it = animations_.begin(); it != animations_.end(); ++it){
		if(it->attr()->playMode() == mode)
			return true;
	}

	return false;
}

bool UI_BackgroundModel::isPlaying(const UI_BackgroundAnimation* animation) const
{
	MTG();
	return (std::find(animations_.begin(), animations_.end(), animation) != animations_.end());
}
	
bool UI_BackgroundModel::play(const UI_BackgroundAnimation* animation, bool reverse)
{
	MTG();
	xassert(animation);
	if(!model_ || animation->duration() < FLT_EPS) return false;

	UI_BackgroundAnimationControllers::iterator it = std::find(animations_.begin(), animations_.end(), animation);

	if(it != animations_.end()){
		bool reverse_mode = reverse ? !animation->reversed() : animation->reversed();
		if(it->reversed() != reverse_mode){
			it->setReversed(reverse_mode);
			it->phaseReverse();
		}

		int groupIndex = model_->GetAnimationGroup(it->attr()->animationGroupName());
		if(groupIndex < 0){
			xassertStr(0, XBuffer() < "� ������������ ������� �������� �������� �������������� ������: " < it->attr()->animationGroupName());
			return false;
		}
//		model_->SetAnimationGroupPhase(groupIndex, it->phase());
		it->setPhase(model_->GetAnimationGroupPhase(groupIndex));
		return true;
	}

	startEffect(&animation->effect());

	animations_.push_back(UI_BackgroundAnimationController(animation));
	if(reverse)
		animations_.back().setReversed(!animation->reversed());

	animations_.back().reset();
	
	int groupIndex = model_->GetAnimationGroup(animations_.back().attr()->animationGroupName());
	if(groupIndex < 0){
		xassertStr(0, XBuffer() < "� ������������ ������� �������� �������� �������������� ������: " < animations_.back().attr()->animationGroupName());
		return false;
	}

	animations_.back().setAnimationGroupIndex(groupIndex);

	model_->SetAnimationGroupChain(groupIndex, animations_.back().attr()->chainName());
//	model_->SetAnimationGroupPhase(groupIndex, animations_.back().phase());
//	animations_.back().setPhase(model_->GetAnimationGroupPhase(groupIndex));

	return true;
}

bool UI_BackgroundModel::stop(const UI_BackgroundAnimation* animation)
{
	MTG();
	xassert(animation);

	stopEffect(&animation->effect());

	UI_BackgroundAnimationControllers::iterator it = std::find(animations_.begin(), animations_.end(), animation);
	if(it != animations_.end()){
		animations_.erase(it);
		return true;
	}

	return false;
}

void UI_BackgroundModel::startEffect(const UI_EffectAttributeAttachable* attr)
{
	xassert(model_);
	
	if(!attr || attr->isEmpty())
		return;

	if(std::find(effects_.begin(), effects_.end(), attr) != effects_.end())
		return;

	UI_EffectControllerAttachable3D effect;
	if(effect.effectStart(attr, model_))
		effects_.push_back(effect);

}

void UI_BackgroundModel::stopEffect(const UI_EffectAttributeAttachable* attr)
{
	xassert(model_);

	if(!attr || attr->isEmpty())
		return;

	Effects::iterator eit = std::find(effects_.begin(), effects_.end(), attr);
	if(eit != effects_.end()){
		eit->effectStop();
		effects_.erase(eit);
	}
}

void UI_BackgroundModel::quant(float dt)
{
	MTG();
	if(!model_) return;	

	UI_BackgroundAnimationControllers::iterator it = animations_.begin();
	while(it != animations_.end()){
		it->quant(dt);
		int animationGroup = it->animationGroupIndex();
		if(animationGroup >= 0){
			model_->SetAnimationGroupChain(animationGroup, it->attr()->chainName());
			model_->SetAnimationGroupPhase(animationGroup, it->phase());
		}
		if(it->isFinished()){
			stopEffect(&it->attr()->effect());
			it = animations_.erase(it);
		}
		else
			++it;
	}
}

void UI_BackgroundModel::drawDebugInfo(Camera* camera) const
{
	Effects::const_iterator eit;
	FOR_EACH(effects_, eit)
		eit->drawDebugInfo(camera);
}

void UI_BackgroundModel::getDebugInfo(Camera* camera, XBuffer& buf) const
{
	MTG();
	for(UI_BackgroundAnimationControllers::const_iterator it = animations_.begin(); it != animations_.end(); ++it){
		char str[64]; str[63] = 0;
		_snprintf(str, 63, "%.4f", it->phase());
		buf < "\n " < it->attr()->animationGroupName() < " / " < it->attr()->chainName() < " " < str;
		if(!it->attr()->effect().isEmpty()){
			buf < "\n  " < it->attr()->effect().effectReference().c_str() < "; ";
			Effects::const_iterator eit = std::find(effects_.begin(), effects_.end(), &it->attr()->effect());
			if(eit != effects_.end()){
				Vect3f pos3d = eit->position();
				_snprintf(str, 63, "world=(%.3f, %.3f, %.3f)", pos3d.x, pos3d.y, pos3d.z);
				buf < str < "; ";
				Vect3f pv, pe;
				camera->ConvertorWorldToViewPort(&pos3d, &pv, &pe);
				_snprintf(str, 63, "sreen=(%d, %d)", pe.xi(), pe.yi());
				buf < str < "\n";
			}
			else
				buf < "NOT ON WORLD";
		}
	}
}

// ------------------------------- UI_BackgroundLight

UI_BackgroundLight::UI_BackgroundLight()
{
	color_ = Color4f(1,1,1,1);
	radius_ = 100.f;

	lifeTime_ = 0.5f;
}

void UI_BackgroundLight::serialize(Archive& ar)
{
	ar.serialize(lifeTime_, "lifeTime", "����� �����");
	ar.serialize(radius_, "radius", "������");
	ar.serialize(color_, "color", "����");
}

// ------------------------------- UI_BackgroundLightController

UI_BackgroundLightController::UI_BackgroundLightController() : light_(0)
{
}

bool UI_BackgroundLightController::start(const UI_BackgroundLight* prm, const Vect3f& position)
{
	release();

	if(light_ = UI_BackgroundScene::instance().scene()->CreateLightDetached(ATTRLIGHT_SPHERICAL_OBJECT)){
		light_->SetDiffuse(prm->color());
		light_->SetRadius(prm->radius());
		light_->SetPosition(MatXf(Mat3f::ID, position));
		light_->Attach();
	}

	lifeTime_ = max(0.1f, prm->lifeTime());

	return true;
}

bool UI_BackgroundLightController::quant(float dt)
{
	lifeTime_ -= dt;
	if(lifeTime_ < 0.f){
		release();
		return false;
	}

	return true;
}

void UI_BackgroundLightController::release()
{
	if(light_){
		light_->Release();
		light_ = 0;
	}
}

// ------------------------------- UI_BackgroundAnimation

UI_BackgroundAnimation::UI_BackgroundAnimation()
{
	playMode_ = PLAY_STARTUP;

	reversed_ = false;
	duration_ = 0.f;
}

void UI_BackgroundAnimation::serialize(Archive& ar)
{
	if(!ar.isEdit()){
		ar.serialize(animationGroupName_, "animationGroupName", "������������ ������");
		ar.serialize(chainName_, "chainName", "�������");
	}
	else {
		ComboListString group_str(UI_BackgroundScene::instance().groupComboList(), animationGroupName_.c_str());
		ar.serialize(group_str, "animationGroupName", "&������������ ������");
		ComboListString chain_str(UI_BackgroundScene::instance().chainComboList(), chainName_.c_str());
		ar.serialize(chain_str, "chainName", "&�������");

		if(ar.isInput()){
			animationGroupName_ = group_str;
			chainName_ = chain_str;
		}
	}

	ar.serialize(effect_, "effect", "������������ ������");

	ar.serialize(playMode_, "playMode", "����� ������������");
	ar.serialize(duration_, "duration", "������������");
	ar.serialize(reversed_, "reversed", "����������� � �������� �������");
}

UI_BackgroundAnimationController::UI_BackgroundAnimationController(const UI_BackgroundAnimation* animation)
{
	animationGroupIndex_ = -1;
	reversed_ = false;
	phase_ = 0.f;
	attr_ = 0;

	if(animation)
		setAnimation(animation);
}

void UI_BackgroundAnimationController::setAnimation(const UI_BackgroundAnimation* animation)
{
	xassert(animation);
	
	attr_ = animation;

	animationGroupIndex_ = -1;
	phase_ = 0.f;

	reversed_ = attr_->reversed();
}

void UI_BackgroundAnimationController::quant(float dt)
{
	MTG();
	xassert(attr_);

	if(attr_->duration() > FLT_EPS)
		phase_ += dt / attr_->duration();

	if(attr_->isCycled())
		phase_ = cycle(phase_, 1.f);
	else
		phase_ = clamp(phase_, 0.f, 1.f);
}

// ------------------------------- UI_BackgroundScene

UI_BackgroundScene::UI_BackgroundScene() :
	scene_(0),
	camera_(0),
	enabled_(true),
	cameraPosition_(0.0f, 0.0f, 1024.f),
	modelPosition_(0.0f, 0.0f, -1024.f),
	cameraAngles_(-180.0f, 0.0f, 0.0f),
	modelAngles_(90.0f, 0.0f, 0.0f),
	lightDirection_(0.f, 2.5f, -2.5f)
{
	cameraFocus_ = scale2focus;
	currentModelIndex_ = -1;
	cameraPerspective_ = false;
}

UI_BackgroundScene::~UI_BackgroundScene()
{
}

void UI_BackgroundScene::init(cVisGeneric* visGeneric)
{
	if(inited())
		done();

	scene_ = visGeneric->CreateScene();

	camera_ = scene_->CreateCamera();

	if(cameraPerspective_)
		camera_->setAttribute(ATTRCAMERA_PERSPECTIVE);

	camera_->setAttribute(ATTRCAMERA_CLEARZBUFFER);
	camera_->setAttribute(ATTRCAMERA_NOCLEARTARGET);

	setCamera();
	
	scene_->SetSunDirection(lightDirection_);

	models_.resize(modelSetups_.size());

	if(currentModelIndex_ != -1){
		models_[currentModelIndex_].load(scene_, modelSetups_[currentModelIndex_], universe() ? universe()->activePlayer() : 0);

		MatXf pos(Mat3f(G2R(modelAngles_.x), X_AXIS) * Mat3f(G2R(modelAngles_.y), Y_AXIS) *
			Mat3f(G2R(modelAngles_.z), Z_AXIS), modelPosition_);

		models_[currentModelIndex_].setPosition(pos);
	}
}

void UI_BackgroundScene::setRenderTarget(cTexture* renderTarget, IDirect3DSurface9* depthBuffer)
{
	xassert(camera_ && renderTarget && depthBuffer);
	camera_->SetRenderTarget(renderTarget, depthBuffer);
}

void UI_BackgroundScene::setCamera()
{
	if(!camera_)
		return;

	Rectf pos;
	float focus = cameraFocus_;

	cTexture* renderTarget = camera_->GetRenderTarget();
	int renderWidth;
	int renderHeight;
	if(renderTarget){
		renderWidth = renderTarget->GetWidth();
		renderHeight = renderTarget->GetHeight();
	}
	else{
		renderWidth = gb_RenderDevice->GetSizeX();
		renderHeight = gb_RenderDevice->GetSizeY();
	}
	float aspect = float(renderWidth) / float(renderHeight);
	if(aspect > 4.0f / 3.0f){
		pos = UI_Render::instance().deviceCoords(Rectf(0,0, renderWidth, renderHeight));
		focus = cameraManager->correctedFocus(focus, camera_);
	}
	else
		pos = UI_Render::instance().deviceCoords(UI_Render::instance().windowPosition());

	MatXf cameraMatrix(Mat3f(G2R(cameraAngles_.x), X_AXIS) * Mat3f(G2R(cameraAngles_.y), Y_AXIS) *
		Mat3f(G2R(cameraAngles_.z), Z_AXIS), cameraPosition_);

	cameraMatrix.Invert();

	camera_->SetPosition(cameraMatrix);

	Vect2f size(pos.width(), pos.height());

	float descale_x = max(1.0f, size.x + 0.0001f);
    float descale_y = max(1.0f, size.y + 0.0001f);

	Vect2f center = pos.center() + Vect2f(0.5f, 0.5f);
	sRectangle4f clip(-size.x / 2 / descale_x, -size.y / 2 / descale_y, 
		               size.x / 2 / descale_x,  size.y / 2 / descale_y);

	camera_->SetFrustum(&center, &clip, &Vect2f(size.x * focus, size.x * focus), 0);
}

void UI_BackgroundScene::setFocus(float focus)
{
	cameraFocus_ = focus;
	if(isUnderEditor() && currentModelIndex_ >= 0)
		modelSetups_[currentModelIndex_].setScale(cameraFocus_ / scale2focus);

	setCamera();
}

void UI_BackgroundScene::setSky(cTexture* texture)
{
	MTG();
	if(scene_)
		scene_->SetSkyCubemap(texture);
}

void UI_BackgroundScene::done()
{
	MTL();
	
	RELEASE(camera_);

	for(UI_BackgroundModels::iterator it = models_.begin(); it != models_.end(); ++it)
		it->release();

	for(LightControllers::iterator il = lightControllers_.begin(); il != lightControllers_.end(); ++il)
		il->release();

	UI_Effects::iterator eit;
	FOR_EACH(effects_, eit)
		eit->effectStop(true);

	effects_.clear();

	models_.clear();
	lightControllers_.clear();

	RELEASE(scene_);
}

bool UI_BackgroundScene::ready() const
{
	return (enabled_ && inited());
}

void UI_BackgroundScene::logicQuant(float dt) 
{
	MTL();

	for(LightControllers::iterator il = lightControllers_.begin(); il != lightControllers_.end();){
		if(!il->quant(dt))
			il = lightControllers_.erase(il);
		else
			++il;
	}
}

void UI_BackgroundScene::graphQuant(float dt) 
{
	if(!ready())
		return;	

	MTG();

	start_timer_auto();

	if(environment){
		scene_->SetSunColor(environment->scene()->GetSunAmbient(),
			environment->scene()->GetSunDiffuse(), environment->scene()->GetSunSpecular());
//		scene_->SetSunDirection(environment->scene()->GetSunDirection());
	}

	for(UI_BackgroundModels::iterator it = models_.begin(); it != models_.end(); ++it){
		it->quant(dt);
		if(it->isLoaded() && !it->isPlaying(UI_BackgroundAnimation::PLAY_STARTUP) && &*it != currentModel())
			it->release();
	}

	scene_->SetDeltaTime(dt*1000.f);

	UI_Effects::iterator eit;
	FOR_EACH(effects_, eit)
		eit->updatePosition();
}

bool UI_BackgroundScene::play(const UI_BackgroundAnimation* animation, bool reverse)
{
	if(UI_BackgroundModel* model = currentModel())
		return model->play(animation, reverse);

	return false;
}

bool UI_BackgroundScene::stop(const UI_BackgroundAnimation* animation)
{
	if(UI_BackgroundModel* model = currentModel())
		return model->stop(animation);

	return false;
}

bool UI_BackgroundScene::startEffect(const UI_EffectAttribute* attr, const UI_ControlBase* owner)
{
	MTG();
	if(!attr || attr->isEmpty())
		return false;

	UI_Effects::iterator it = std::find(effects_.begin(), effects_.end(), owner);
	if(it == effects_.end()){
		UI_EffectControllerAttachable2D effect;
		if(effect.effectStart(attr, owner)){
			effects_.push_back(effect);
			return true;
		}
		return false;
	}
	else if(!safe_cast_ref<const UI_EffectController&>(*it).operator ==(attr)){
		if(!it->effectStart(attr, owner)){
			effects_.erase(it);
			return false;
		}
	}
	return true;
}

void UI_BackgroundScene::stopEffect(const UI_ControlBase* owner, bool immediately)
{
	MTG();
	xassert(owner);

	UI_Effects::iterator eit = std::find(effects_.begin(), effects_.end(), owner);
	if(eit != effects_.end()){
		eit->effectStop(immediately);
		effects_.erase(eit);
	}
}

bool UI_BackgroundScene::isPlaying() const
{
	if(const UI_BackgroundModel* model = currentModel())
		return model->isPlaying();

	return false;
}

bool UI_BackgroundScene::isPlaying(UI_BackgroundAnimation::PlayMode mode) const
{
	if(const UI_BackgroundModel* model = currentModel())
		return model->isPlaying(mode);

	return false;
}

bool UI_BackgroundScene::isPlaying(const UI_BackgroundAnimation* animation) const
{
	if(const UI_BackgroundModel* model = currentModel())
		return model->isPlaying(animation);

	return false;
}

bool UI_BackgroundScene::addLight(int light_index, const Vect2f& position)
{
	MTL();

	if(!ready())
		return false;

	lightControllers_.push_back(UI_BackgroundLightController());

	Vect3f pos;
	camera_->ConvertorCameraToWorld(position, pos);

	if(light_index >= 0 && light_index < lights_.size())
		return lightControllers_.back().start(&lights_[light_index], pos);

	UI_BackgroundLight light;
	return lightControllers_.back().start(&light, pos);
}

int UI_BackgroundScene::getModelIndex(const char* modelName) const
{
	if(!modelName || !*modelName)
		return -1;
	UI_BackgroundModelSetups::const_iterator it = std::find(modelSetups_.begin(), modelSetups_.end(), modelName);
	return it != modelSetups_.end() ? std::distance(modelSetups_.begin(), it) : -1;
}

void UI_BackgroundScene::selectModel(const char* model_name)
{
	currentModelIndex_ = getModelIndex(model_name);

	if(currentModelIndex_ >= 0){
		if(!models_[currentModelIndex_].isLoaded()){
			models_[currentModelIndex_].load(scene_, modelSetup(currentModelIndex_), universe() ? universe()->activePlayer() : 0);
			scene_->HideAllObjectLights(false);

			MatXf pos(Mat3f(G2R(modelAngles_.x), X_AXIS) * Mat3f(G2R(modelAngles_.y), Y_AXIS) *
				Mat3f(G2R(modelAngles_.z), Z_AXIS), modelPosition_);

			models_[currentModelIndex_].setPosition(pos);
		}

		cameraFocus_ = modelSetups_[currentModelIndex_].scale() * scale2focus;
	}
	else if(model_name)
		cameraFocus_ = scale2focus;

	setCamera();
}

void UI_BackgroundScene::draw() const
{
	if(!ready())
		return;	

	start_timer_auto();
	scene_->Draw(camera_);
}

void UI_BackgroundScene::serialize(Archive& ar)
{
	ar.serialize(enabled_, "enabled", "�������");

	ar.serialize(modelSetups_, "models", "������");

	if(ar.isInput())
		updateModelComboList();

	ar.serialize(lightDirection_, "lightDirection", "����������� ���������");
	ar.serialize(lights_, "lights", "��������� �����");

	//if(ar.openBlock("camera", "������")){
	//	ar.serialize(cameraPosition_, "position", "�������");
	//	ar.serialize(cameraAngles_, "angles", "�������");
	//	ar.serialize(cameraFocus_, "focusx", "�����");
	//	ar.serialize(cameraPerspective_, "perspective", "�����������");

	//	ar.closeBlock();
	//}
}

const char* UI_BackgroundScene::groupComboList() const
{
	if(const UI_BackgroundModelSetup* setup = currentModelSetup())
		return setup->groupComboList();

	return "";
}

const char* UI_BackgroundScene::chainComboList() const
{
	if(const UI_BackgroundModelSetup* setup = currentModelSetup())
		return setup->chainComboList();

	return "";
}

const char* UI_BackgroundScene::nodeComboList() const
{
	if(const UI_BackgroundModelSetup* setup = currentModelSetup())
		return setup->nodeComboList();

	return "";
}

void UI_BackgroundScene::drawDebugInfo() const
{
	if(showDebugInterface.background)
		for(UI_BackgroundModels::const_iterator it = models_.begin(); it != models_.end(); ++it)
			it->drawDebugInfo(camera_);
		
	if(showDebugInterface.bgeffects){
		UI_Effects::const_iterator eit;
		FOR_EACH(effects_, eit)
			eit->drawDebugInfo(camera_);
	}
}

void UI_BackgroundScene::drawDebug2D() const
{
	XBuffer buf;

	if(showDebugInterface.bgeffects){
		buf < "UI_Background Effects:\n";
		UI_Effects::const_iterator eit;
		FOR_EACH(effects_, eit){
			eit->getDebugInfo(camera_, buf);
			buf < "\n";
		}
	}

	if(showDebugInterface.background){
		if(buf.tell())
			buf < "----------------------------\n";
		buf < "UI_BackgroundScene: scale = " <= cameraFocus_ / scale2focus;
		for(UI_BackgroundModels::const_iterator it = models_.begin(); it != models_.end(); ++it){
			if(it->isPlaying()){
				buf < "\n model: " < it->model()->GetFileName();
				it->getDebugInfo(camera_, buf);
			}
		}
	}

	if(buf.tell())
		UI_Render::instance().outDebugText(Vect2f(0.f, 0.15f), buf);
}

void UI_BackgroundScene::updateModelComboList()
{
	modelComboList_.clear();

	for(UI_BackgroundModelSetups::iterator it = modelSetups_.begin(); it != modelSetups_.end(); ++it){
		modelComboList_ += "|";
		modelComboList_ += it->modelName();
	}
}

