#ifndef _PERIMETER_COMMON_EVENTS_
#define _PERIMETER_COMMON_EVENTS_

#include "NetPlayer.h"
#include "MissionDescriptionNet.h"
#include "ConnectionInfo.h"
#include "GameCommands.h"
#include "StringWrappers.h"
//-------------------------------

template<NCEventID EVENT_ID>
class NetCommandGeneral : public NetCommandBase{
public:
	NetCommandGeneral():NetCommandBase(EVENT_ID) {}
};

//-------------------------------------------

struct netCommand4C_StartLoadGame : public NetCommandGeneral<NETCOM4C_StartLoadGame> {

	MissionDescription missionDescription_;
	netCommand4C_StartLoadGame(MissionDescription& missionDescription, const XBuffer& data) {
		missionDescription_=missionDescription;
		dataSizeInAuxBuf = data.tell();
		xassert(dataSizeInAuxBuf <= sizeof(auxBuf));
		if(dataSizeInAuxBuf <= sizeof(auxBuf))
			memcpy(auxBuf, data.buffer(), dataSizeInAuxBuf);
        else
			dataSizeInAuxBuf=0;
	}
	netCommand4C_StartLoadGame(XBuffer& in) { 
		missionDescription_.readNet(in);
		in.read(&dataSizeInAuxBuf, sizeof(dataSizeInAuxBuf));
		xassert(dataSizeInAuxBuf <= sizeof(auxBuf));
		if(dataSizeInAuxBuf <= sizeof(auxBuf))
            in.read(&auxBuf[0], dataSizeInAuxBuf);
		else
			dataSizeInAuxBuf=0;
	}
	void Write(XBuffer& out) const { 
		missionDescription_.writeNet(out);
		out.write(&dataSizeInAuxBuf, sizeof(dataSizeInAuxBuf));
		out.write(&auxBuf[0], dataSizeInAuxBuf);
	}
	short dataSizeInAuxBuf;
	unsigned char auxBuf[512];
};

////////////////////////////////////
//CHAT
class ChatMessage{
public:
	enum { MAX_SIZE_MESSAGE=128 };
	enum { DEFAULT_ID=-1 };
	ChatMessage() { immediatelySize = sizeof(ChatMessage); id=DEFAULT_ID; data[0]='\0'; }
	ChatMessage(const char* _str, signed char _id) { 
		dataSize_=strlen(_str)+1;
		if(dataSize_>MAX_SIZE_MESSAGE) dataSize_=MAX_SIZE_MESSAGE;
		memcpy(data, _str, dataSize_-1);
		data[dataSize_-1]=0;

		immediatelySize = sizeof(ChatMessage); 
		id=_id; 
	}
	bool read(XBuffer& in){
		in.read(&dataSize_, sizeof(dataSize_));
		if(dataSize_>0 && dataSize_<=MAX_SIZE_MESSAGE){
			in.read(data, dataSize_);
			in.read(&immediatelySize, sizeof(immediatelySize));
			if(immediatelySize==sizeof(ChatMessage))
				in.read(&id, sizeof(id)); //additional info
			else 
				id=DEFAULT_ID;
			data[dataSize_-1]=0;
			return true;
		}
		else {
			xassert(0); dataSize_=0; data[0]=0;
			return false;
		}

	}
	bool read(class bdBitBuffer& bb);
	void write(class bdBitBuffer& bb);
	void write(XBuffer& out) const{
		out.write(&dataSize_, sizeof(dataSize_));
		xassert(dataSize_<=MAX_SIZE_MESSAGE);
		out.write(data, dataSize_);
		out.write(&immediatelySize, sizeof(immediatelySize));
		out.write(&id, sizeof(id)); //additional info
	}

	const char* getText() const {
		ChatMessage& im=const_cast<ChatMessage&>(*this);
		if(dataSize_>0 && dataSize_<=MAX_SIZE_MESSAGE)
			im.data[dataSize_-1]='\0';
		else 
            im.data[0]='\0';
		return data;
	}
	signed char id;
protected:
	unsigned int dataSize_;
	char data[MAX_SIZE_MESSAGE];
    unsigned short immediatelySize;
};

class netCommand4G_ChatMessage : 
	public netCommandGameClonator<netCommand4G_ChatMessage, NETCOM4G_ChatMessage>
{
public:

	netCommand4G_ChatMessage(const ChatMessage& _chatMsg){
		chatMsg=_chatMsg;
	}
	netCommand4G_ChatMessage(XBuffer& in){ 
		baseRead(in);
		chatMsg.read(in);
	}
	void Write(XBuffer& out) const{ //virtual
		baseWrite(out);
		chatMsg.write(out);
	}
	bool isGameCommand() const { return false; }
	void execute() const { xassert(0); }
	bool compare(const netCommandGame& sop) const { xassert(0); return false; }
	ChatMessage chatMsg;
};

////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
struct netCommand4H_BackGameInformation : public NetCommandGeneral<NETCOM4H_BackGameInformation> {
public:
	netCommand4H_BackGameInformation (unsigned int quant, XBuffer& vmapData, XBuffer& gameData){ // : NetCommandBase(NETCOM4H_BackGameInformation){
		quant_=quant;
		VDataSize_=vmapData.tell();
		pVData_=new unsigned char[VDataSize_];
		memcpy(pVData_, vmapData, VDataSize_);
		GDataSize_=gameData.tell();
		pGData_=new unsigned char[GDataSize_];
		memcpy(pGData_, gameData, GDataSize_);
	}
	netCommand4H_BackGameInformation (XBuffer& in) { //: NetCommandBase(NETCOM4H_BackGameInformation)
		in.read(&quant_, sizeof(quant_));
		in.read(&VDataSize_, sizeof(VDataSize_));
		pVData_=new unsigned char[VDataSize_];
		in.read(pVData_, VDataSize_);
		in.read(&GDataSize_, sizeof(GDataSize_));
		pGData_=new unsigned char[GDataSize_];
		in.read(pGData_, GDataSize_);
	}
	~netCommand4H_BackGameInformation (){
		delete pVData_;
		delete pGData_;
	}
	
	void Write(XBuffer& out) const{
		out.write(&quant_, sizeof(quant_));
		out.write(&VDataSize_, sizeof(VDataSize_));
		out.write(pVData_, VDataSize_);
		out.write(&GDataSize_, sizeof(GDataSize_));
		out.write(pGData_, GDataSize_);
	}

	bool operator == (const netCommand4H_BackGameInformation &secop) const {
		return ( (quant_ == secop.quant_) && 
			(VDataSize_== secop.VDataSize_) && (memcmp(pVData_, secop.pVData_, VDataSize_)==0) &&
			(GDataSize_== secop.GDataSize_) && (memcmp(pGData_, secop.pGData_, GDataSize_)==0) );
	}
	bool equalGData(const netCommand4H_BackGameInformation &secop){
		return ( (GDataSize_== secop.GDataSize_) && (memcmp(pGData_, secop.pGData_, GDataSize_)==0) );
	};
	bool equalVData(const netCommand4H_BackGameInformation &secop){
		return ( (VDataSize_== secop.VDataSize_) && (memcmp(pVData_, secop.pVData_, VDataSize_)==0) );
	};
	unsigned int quant_;
	unsigned int VDataSize_;
	unsigned char* pVData_;
	unsigned int GDataSize_;
	unsigned char* pGData_;
};


struct netCommand4H_BackGameInformation2 : public NetCommandGeneral<NETCOM4H_BackGameInformation2> {
public:
	netCommand4H_BackGameInformation2 (unsigned int lagQuant, unsigned int quant, unsigned int signature, unsigned int accessibleLogicQuantPeriod, bool replay=0, int state=0){// : NetCommandBase(NETCOM4H_BackGameInformation2)
		backGameInf2_.lagQuant_=lagQuant;
		backGameInf2_.quant_=quant;
		backGameInf2_.signature_=signature;
		backGameInf2_.replay_=replay;
		backGameInf2_.state_=state;
		backGameInf2_.accessibleLogicQuantPeriod_=accessibleLogicQuantPeriod;
	}
	netCommand4H_BackGameInformation2 (XBuffer& in){// : NetCommandBase(NETCOM4H_BackGameInformation2)
		//in.read(&lagQuant_, sizeof(lagQuant_));
		//in.read(&quant_, sizeof(quant_));
		//in.read(&signature_, sizeof(signature_));
		//in.read(&accessibleLogicQuantPeriod_, sizeof(accessibleLogicQuantPeriod_));
		//in.read(&replay_, sizeof(replay_));
		//in.read(&state_, sizeof(state_));
		in.read(&backGameInf2_, sizeof(backGameInf2_));
	}

	void Write(XBuffer& out) const{
		//out.write(&lagQuant_, sizeof(lagQuant_));
		//out.write(&quant_, sizeof(quant_));
		//out.write(&signature_, sizeof(signature_));
		//out.write(&accessibleLogicQuantPeriod_, sizeof(accessibleLogicQuantPeriod_));
		//out.write(&replay_, sizeof(replay_));
		//out.write(&state_, sizeof(state_));
		out.write(&backGameInf2_, sizeof(backGameInf2_));
	}

	BackGameInformation2 backGameInf2_;
};


struct netCommand4C_DisplayDistrincAreas : public NetCommandGeneral<NETCOM4C_DisplayDistrincAreas> {
public:
	netCommand4C_DisplayDistrincAreas(bool tmp, XBuffer& distrincAreas){// : NetCommandBase(NETCOM4C_DisplayDistrincAreas)
		DASize_=distrincAreas.tell();
		pDAData_=new unsigned char[DASize_];
		memcpy(pDAData_, distrincAreas, DASize_);
	}
	netCommand4C_DisplayDistrincAreas(XBuffer& in){// : NetCommandBase(NETCOM4C_DisplayDistrincAreas)
		in.read(&DASize_, sizeof(DASize_));
		pDAData_=new unsigned char[DASize_];
		in.read(pDAData_, DASize_);
	}
	~netCommand4C_DisplayDistrincAreas(){
		delete pDAData_;
	}
	
	void Write(XBuffer& out) const{
		out.write(&DASize_, sizeof(DASize_));
		out.write(pDAData_, DASize_);
	}

	unsigned int DASize_;
	unsigned char* pDAData_;
};

struct netCommand4C_SaveLog : public NetCommandGeneral<NETCOM4C_SaveLog> {
public:
	netCommand4C_SaveLog(int errorQuant){// : NetCommandBase(NETCOM4C_SaveLog)
		quant=errorQuant;
	}
	netCommand4C_SaveLog(XBuffer& in){// : NetCommandBase(NETCOM4C_SaveLog)
		in > quant;
	}
	void Write(XBuffer& out) const {
		out < quant;
	};
	int quant;
};

struct netCommand4C_sendLog2Host : public NetCommandGeneral<NETCOM4C_SendLog2Host> {
public:
	netCommand4C_sendLog2Host(unsigned int _begQuant){// : NetCommandBase(NETCOM4C_SendLog2Host)
		begQuant=_begQuant;
	}
	netCommand4C_sendLog2Host (XBuffer& in){// : NetCommandBase(NETCOM4C_SendLog2Host)
		in > begQuant;
	}
	void Write(XBuffer& out) const {
		out < begQuant;
	};
	unsigned int begQuant;
};


//----------------------------------------------------
struct netCommand4H_RequestPause : NetCommandGeneral<NETCOM4H_RequestPause>
{
	int playerID;
	bool pause;
	netCommand4H_RequestPause(int _playerID, bool _pause){// : NetCommandBase(NETCOM4H_RequestPause)
		playerID=_playerID;
		pause=_pause;
	}
	netCommand4H_RequestPause(XBuffer& in){// : NetCommandBase(NETCOM4H_RequestPause)
		in.read(&playerID, sizeof(playerID));
		in.read(&pause, sizeof(pause));
	}
	void Write(XBuffer& out) const {
		out.write(&playerID, sizeof(playerID));
		out.write(&pause, sizeof(pause));
	}
};

struct netCommand4C_Pause : NetCommandGeneral<NETCOM4C_Pause>
{
	enum { NOT_PLAYER_IDX=-1 };
	int usersIdxArr[NETWORK_PLAYERS_MAX];
	bool pause;
	netCommand4C_Pause(const int _usersIdxArr[NETWORK_PLAYERS_MAX], bool _pause){// : NetCommandBase(NETCOM4C_Pause)
		memcpy(usersIdxArr, _usersIdxArr, sizeof(usersIdxArr));
		pause=_pause;
	}
	netCommand4C_Pause(XBuffer& in){// : NetCommandBase(NETCOM4C_Pause)
		in.read(&usersIdxArr[0], sizeof(usersIdxArr));
		in.read(&pause, sizeof(pause));
	}
	void Write(XBuffer& out) const {
		out.write(&usersIdxArr[0], sizeof(usersIdxArr));
		out.write(&pause, sizeof(pause));
	}

};

struct netCommandNextQuant : NetCommandGeneral<NETCOM4G_NextQuant>
{
	enum { NOT_QUANT_CONFIRMATION=-1 };
	unsigned int numberQuant_;
	unsigned int amountCommandsPerQuant_;
	unsigned int quantConfirmation_;
	unsigned long globalCommandCounter_;
	bool flag_pause_;
	unsigned int quantInterval_;


	netCommandNextQuant(unsigned int numberQuant, unsigned int quantInterval, unsigned int amountCommandsPerQuant, unsigned long globalCommandCounter, unsigned int quantConfirmation=NOT_QUANT_CONFIRMATION, bool flag_pause=0){// : NetCommandBase(NETCOM4G_NextQuant)
		numberQuant_=numberQuant;
		amountCommandsPerQuant_=amountCommandsPerQuant;
		quantConfirmation_=quantConfirmation;
		globalCommandCounter_=globalCommandCounter;
		flag_pause_=flag_pause;
		quantInterval_=quantInterval;
	}
	netCommandNextQuant(XBuffer& in){// : NetCommandBase(NETCOM4G_NextQuant)
		in.read(&numberQuant_, sizeof(numberQuant_));
		in.read(&amountCommandsPerQuant_, sizeof(amountCommandsPerQuant_));
		in.read(&quantConfirmation_, sizeof(quantConfirmation_));
		in.read(&globalCommandCounter_, sizeof(globalCommandCounter_));
		in.read(&flag_pause_, sizeof(flag_pause_));
		in.read(&quantInterval_, sizeof(quantInterval_));
	}

	void Write(XBuffer& out) const {
		out.write(&numberQuant_, sizeof(numberQuant_));
		out.write(&amountCommandsPerQuant_, sizeof(amountCommandsPerQuant_));
		out.write(&quantConfirmation_, sizeof(quantConfirmation_));
		out.write(&globalCommandCounter_, sizeof(globalCommandCounter_));
		out.write(&flag_pause_, sizeof(flag_pause_));
		out.write(&quantInterval_, sizeof(quantInterval_));
	}
};


//------------------------------
//������ ���������� �� ������������ �.�. ����������� � ���������� ������ - ExecuteInternalCommand(PNC_COMMAND__START_HOST_AND_CREATE_GAME_AND_STOP_FIND_HOST, true);
//struct netCommand4H_CreateGame : NetCommandGeneral<NETCOM4H_CreateGame> {
//	MissionDescription missionDescription_;
//	PlayerData createPlayerData_;
//	char gameName_[MAX_MULTIPALYER_GAME_NAME];
//	char computerName_[MAX_COMPUTERNAME_LENGTH+1];
//	unsigned int internalNumverVersion_;
//	char simpleGameVersion_[sizeof(SIMPLE_GAME_CURRENT_VERSION)];
//
//	netCommand4H_CreateGame(const char* gameName, const char * computerName, MissionDescription& missionDescription, PlayerData& createPlayerData){// : NetCommandBase(NETCOM4H_CreateGame)
//		strncpy(gameName_, gameName, MAX_MULTIPALYER_GAME_NAME);
//		strncpy(computerName_, computerName, MAX_COMPUTERNAME_LENGTH+1);
//		missionDescription_=missionDescription;
//		createPlayerData_=createPlayerData;
//		internalNumverVersion_=INTERNAL_BUILD_VERSION;
//		strncpy(simpleGameVersion_, SIMPLE_GAME_CURRENT_VERSION, sizeof(simpleGameVersion_));
//	}
//
//	netCommand4H_CreateGame(XBuffer& in){// : NetCommandBase(NETCOM4H_CreateGame)
//		in.read(gameName_, MAX_MULTIPALYER_GAME_NAME);
//		in.read(computerName_, MAX_COMPUTERNAME_LENGTH+1);
//		missionDescription_.readNet(in);
//		in.read(&createPlayerData_, sizeof(createPlayerData_));
//		in.read(&internalNumverVersion_, sizeof(internalNumverVersion_));
//		in.read(&simpleGameVersion_, sizeof(simpleGameVersion_));
//	}
//	void Write(XBuffer& out) const {
//		out.write(gameName_, MAX_MULTIPALYER_GAME_NAME);
//		out.write(computerName_, MAX_COMPUTERNAME_LENGTH+1);
//		missionDescription_.writeNet(out);
//		out.write(&createPlayerData_, sizeof(createPlayerData_));
//		out.write(&internalNumverVersion_, sizeof(internalNumverVersion_));
//		out.write(&simpleGameVersion_, sizeof(simpleGameVersion_));
//	}
//};


struct netCommandC_JoinRequest : NetCommandGeneral<NETCOM4H_JoinRequest>
{
	sConnectInfo connectInfo;
	netCommandC_JoinRequest(const sConnectInfo& _connectInfo){
		connectInfo=_connectInfo;
	}
	netCommandC_JoinRequest(XBuffer& in){
		in.read(&connectInfo, sizeof(connectInfo));
	}
	void Write(XBuffer& out) const{
		out.write(&connectInfo, sizeof(connectInfo));
	}
};

struct netCommand4C_JoinResponse : NetCommandGeneral<NETCOM4C_JoinResponse>
{
	sReplyConnectInfo replyConnectInfo;
	netCommand4C_JoinResponse(const sReplyConnectInfo&  _replyConnectInfo){
		replyConnectInfo=_replyConnectInfo;
	}
	netCommand4C_JoinResponse(XBuffer& in){
		in.read(&replyConnectInfo, sizeof(replyConnectInfo));
	}
	void Write(XBuffer& out) const {
		out.write(&replyConnectInfo, sizeof(replyConnectInfo));
	}
};




struct netCommand4H_ReJoinRequest : NetCommandGeneral<NETCOM4H_RejoinRequest>
{
	unsigned int currentLastQuant;
	unsigned int confirmedQuant;
	netCommand4H_ReJoinRequest(unsigned int _currentLastQuant, unsigned int _confirmedQuant){// : NetCommandBase(NETCOM4H_RejoinRequest) 
		currentLastQuant=_currentLastQuant;
		confirmedQuant=_confirmedQuant;
	}
	netCommand4H_ReJoinRequest(XBuffer& in){// : NetCommandBase(NETCOM4H_RejoinRequest)
		in.read(&currentLastQuant, sizeof(currentLastQuant));
		in.read(&confirmedQuant, sizeof(confirmedQuant));
	}

	void Write(XBuffer& out) const{
		out.write(&currentLastQuant, sizeof(currentLastQuant));
		out.write(&confirmedQuant, sizeof(confirmedQuant));
	}
};


struct netCommand4C_RequestLastQuantsCommands : NetCommandGeneral<NETCOM4C_RequestLastQuantCommands>
{
	unsigned int beginQunat_;
	netCommand4C_RequestLastQuantsCommands(unsigned int beginQunat){// : NetCommandBase(NETCOM4C_RequestLastQuantCommands)
		beginQunat_=beginQunat;
	}
	netCommand4C_RequestLastQuantsCommands(XBuffer& in){// : NetCommandBase(NETCOM4C_RequestLastQuantCommands)
		in.read(&beginQunat_, sizeof(beginQunat_));
	}

	void Write(XBuffer& out) const{
		out.write(&beginQunat_, sizeof(beginQunat_));
	}
};

struct netCommand4H_ResponceLastQuantsCommands : NetCommandGeneral<NETCOM4H_ResponceLastQuantsCommands>
{
	unsigned int beginQuantCommandTransmit;
	unsigned int endQuantCommandTransmit;
	unsigned int finGeneraCommandCounter;
	unsigned int sizeCommandBuf;
	unsigned char* pData;

	netCommand4H_ResponceLastQuantsCommands(unsigned int _bQC, unsigned int _eQC, unsigned int _fGCC, unsigned int _szBuf, unsigned char* _pData){// : NetCommandBase(NETCOM4H_ResponceLastQuantsCommands)
		beginQuantCommandTransmit=_bQC;
		endQuantCommandTransmit=_eQC;
		finGeneraCommandCounter=_fGCC;
		sizeCommandBuf=_szBuf;
		pData= new unsigned char[sizeCommandBuf];
		memcpy(pData, _pData, sizeCommandBuf);
	}
	netCommand4H_ResponceLastQuantsCommands(XBuffer& in){// : NetCommandBase(NETCOM4H_ResponceLastQuantsCommands)
		in.read(&beginQuantCommandTransmit, sizeof(beginQuantCommandTransmit));
		in.read(&endQuantCommandTransmit, sizeof(endQuantCommandTransmit));
		in.read(&finGeneraCommandCounter,sizeof(finGeneraCommandCounter));
		in.read(&sizeCommandBuf, sizeof(sizeCommandBuf));
		pData= new unsigned char[sizeCommandBuf];
		in.read(pData, sizeCommandBuf);
	}
	~netCommand4H_ResponceLastQuantsCommands(){
		delete pData;
	}
	void Write(XBuffer& out) const{
		out.write(&beginQuantCommandTransmit, sizeof(beginQuantCommandTransmit));
		out.write(&endQuantCommandTransmit, sizeof(endQuantCommandTransmit));
		out.write(&finGeneraCommandCounter,sizeof(finGeneraCommandCounter));
		out.write(&sizeCommandBuf, sizeof(sizeCommandBuf));
		out.write(pData, sizeCommandBuf);
	}
};


struct netCommandC_PlayerReady : NetCommandGeneral<NETCOM4H_GameIsLoaded>
{
	unsigned int gameCRC_;
	netCommandC_PlayerReady(unsigned int gameCRC){// : NetCommandBase(NETCOM4H_GameIsLoaded)
		gameCRC_=gameCRC;
	}
	netCommandC_PlayerReady(XBuffer& in){// : NetCommandBase(NETCOM4H_GameIsLoaded)
		in.read(&gameCRC_, sizeof(gameCRC_));
	}
	void Write(XBuffer& out) const {
		out.write(&gameCRC_, sizeof(gameCRC_));
	}
};

struct netCommand4C_AlifePacket : NetCommandGeneral<NETCOM4C_AlifePacket>
{
	netCommand4C_AlifePacket(){}// : NetCommandBase(NETCOM4C_AlifePacket)
	netCommand4C_AlifePacket(XBuffer& in){}// : NetCommandBase(NETCOM4C_AlifePacket)
};

struct netCommand4H_AlifePacket : NetCommandGeneral<NETCOM4H_AlifePacket>
{
	netCommand4H_AlifePacket(){}// : NetCommandBase(NETCOM4H_AlifePacket)
	netCommand4H_AlifePacket(XBuffer& in){}// : NetCommandBase(NETCOM4H_AlifePacket)
};


struct netCommand4C_ClientIsNotResponce : NetCommandGeneral<NETCOM4C_ClientIsNotResponce>
{
	netCommand4C_ClientIsNotResponce(string& _clientNotResponceList){// : NetCommandBase(NETCOM4C_ClientIsNotResponce)
		clientNotResponceList=_clientNotResponceList;
	}
	netCommand4C_ClientIsNotResponce(XBuffer& in){// : NetCommandBase(NETCOM4C_ClientIsNotResponce)
		in > StringInWrapper(clientNotResponceList);
	}
	void Write(XBuffer& out) const {
		out < StringOutWrapper(clientNotResponceList);
	}
	string clientNotResponceList;
};




struct netCommand4C_ContinueGameAfterHostMigrate : NetCommandGeneral<NETCOM4C_ContinueGameAfterHostMigrate>
{
	netCommand4C_ContinueGameAfterHostMigrate(){}// : NetCommandBase(NETCOM4C_ContinueGameAfterHostMigrate)
	netCommand4C_ContinueGameAfterHostMigrate(XBuffer& in){}// : NetCommandBase(NETCOM4C_ContinueGameAfterHostMigrate)
	//void Write(XBuffer& out) const;
};

struct netCommand4H_PlayerIsReadyOrStartLoadGame : NetCommandGeneral<NETCOM4H_PlayerIsReadyOrStartLoadGame>
{
	netCommand4H_PlayerIsReadyOrStartLoadGame(){
		v=0;
	}
	netCommand4H_PlayerIsReadyOrStartLoadGame(XBuffer& in){
		in > v;
	}
	void Write(XBuffer& out) const {
		out < v;
	};
	int v;
};

struct netCommand4C_CurrentMissionDescriptionInfo : NetCommandGeneral<NETCOM4C_CurMissionDescriptionInfo>
{
	MissionDescription missionDescription_;

	netCommand4C_CurrentMissionDescriptionInfo(MissionDescription& missionDescription){// : NetCommandBase(NETCOM4C_CurMissionDescriptionInfo)
		missionDescription_=missionDescription;
	}

	netCommand4C_CurrentMissionDescriptionInfo(XBuffer& in){// : NetCommandBase(NETCOM4C_CurMissionDescriptionInfo)
		missionDescription_.readNet(in);
	}

	void Write(XBuffer& out) const {
		missionDescription_.writeNet(out);
	}
};

struct netCommand4H_ChangePlayerRace : NetCommandGeneral<NETCOM4H_ChangePlayerRace>
{
	netCommand4H_ChangePlayerRace(int slotID, Race newRace){// : NetCommandBase(NETCOM4H_ChangePlayerRace)
		slotID_=slotID;
		newRace_=newRace;
	}
	netCommand4H_ChangePlayerRace(XBuffer& in){// : NetCommandBase(NETCOM4H_ChangePlayerRace)
		in.read(&slotID_, sizeof(slotID_));
		in.read(&newRace_, sizeof(newRace_));
	}
	void Write(XBuffer& out) const {
		out.write(&slotID_, sizeof(slotID_));
		out.write(&newRace_, sizeof(newRace_));
	};
	int slotID_;
	Race newRace_;
};

struct netCommand4H_ChangePlayerColor : NetCommandGeneral<NETCOM4H_ChangePlayerColor>
{
	netCommand4H_ChangePlayerColor(int idxPlayerData, int newColor){// : NetCommandBase(NETCOM4H_ChangePlayerColor)
		slotID_=idxPlayerData;
		newColor_=newColor;
	}
	netCommand4H_ChangePlayerColor(XBuffer& in){// : NetCommandBase(NETCOM4H_ChangePlayerColor)
		in.read(&slotID_, sizeof(slotID_));
		in.read(&newColor_, sizeof(newColor_));
	}
	void Write(XBuffer& out) const {
		out.write(&slotID_, sizeof(slotID_));
		out.write(&newColor_, sizeof(newColor_));
	};
	int slotID_;
	int newColor_;
};

struct netCommand4H_ChangePlayerSign : NetCommandGeneral<NETCOM4H_ChangePlayerSign>
{
	netCommand4H_ChangePlayerSign(int slotID, int newSign){// : NetCommandBase(NETCOM4H_ChangePlayerSign)
		slotID_=slotID;
		sign_=newSign;
	}
	netCommand4H_ChangePlayerSign(XBuffer& in){// : NetCommandBase(NETCOM4H_ChangePlayerSign)
		in.read(&slotID_, sizeof(slotID_));
		in.read(&sign_, sizeof(sign_));
	}
	void Write(XBuffer& out) const {
		out.write(&slotID_, sizeof(slotID_));
		out.write(&sign_, sizeof(sign_));
	};
	int slotID_;
	int sign_;
};


struct netCommand4H_ChangeRealPlayerType : NetCommandGeneral<NETCOM4H_ChangeRealPlayerType>
{
	netCommand4H_ChangeRealPlayerType(int _slotID, RealPlayerType newRealPlayerType){
		slotID_=_slotID;
		newRealPlayerType_=newRealPlayerType;
	}
	netCommand4H_ChangeRealPlayerType(XBuffer& in){
		in.read(&slotID_, sizeof(slotID_));
		in.read(&newRealPlayerType_, sizeof(newRealPlayerType_));
	}
	void Write(XBuffer& out) const {
		out.write(&slotID_, sizeof(slotID_));
		out.write(&newRealPlayerType_, sizeof(newRealPlayerType_));
	};
	int slotID_;
	RealPlayerType newRealPlayerType_;
};

struct netCommand4H_ChangePlayerDifficulty : NetCommandGeneral<NETCOM4H_ChangePlayerDifficulty>
{
	netCommand4H_ChangePlayerDifficulty(int slotID, Difficulty difficulty){// : NetCommandBase(NETCOM4H_ChangePlayerDifficulty)
		slotID_=slotID;
		difficulty_=difficulty;
	}
	netCommand4H_ChangePlayerDifficulty(XBuffer& in){// : NetCommandBase(NETCOM4H_ChangePlayerDifficulty)
		in.read(&slotID_, sizeof(slotID_));
		in.read(&difficulty_, sizeof(difficulty_));
	}
	void Write(XBuffer& out) const {
		out.write(&slotID_, sizeof(slotID_));
		out.write(&difficulty_, sizeof(difficulty_));
	};
	int slotID_;
	Difficulty difficulty_;
};
struct netCommand4H_ChangePlayerClan : NetCommandGeneral<NETCOM4H_ChangePlayerClan>
{
	netCommand4H_ChangePlayerClan(int slotID, int clan){// : NetCommandBase(NETCOM4H_ChangePlayerClan)
		slotID_=slotID;
		clan_=clan;
	}
	netCommand4H_ChangePlayerClan(XBuffer& in){// : NetCommandBase(NETCOM4H_ChangePlayerClan)
		in.read(&slotID_, sizeof(slotID_));
		in.read(&clan_, sizeof(clan_));
	}
	void Write(XBuffer& out) const {
		out.write(&slotID_, sizeof(slotID_));
		out.write(&clan_, sizeof(clan_));
	};
	int slotID_;
	int clan_;
};

struct netCommand4H_ChangeMD : NetCommandGeneral<NETCOM4H_ChangeMissionDescription>
{
	netCommand4H_ChangeMD(MissionDescriptionNet::eChangedMDVal _val, int _v){
		chv=_val;
		v=_v;
	}
	netCommand4H_ChangeMD(XBuffer& in){
		in.read(&chv, sizeof(chv));
		in.read(&v, sizeof(v));
	}
	void Write(XBuffer& out) const {
		out.write(&chv, sizeof(chv));
		out.write(&v, sizeof(v));
	}
	MissionDescriptionNet::eChangedMDVal chv;
	int v;
};

struct netCommand4H_Join2Command : NetCommandGeneral<NETCOM4H_Join2Command>
{
	netCommand4H_Join2Command(int idxUserData,int commandID){
		idxUserData_=idxUserData;
		commandID_=commandID;
	}
	netCommand4H_Join2Command(XBuffer& in){
		in.read(&idxUserData_, sizeof(idxUserData_));
		in.read(&commandID_, sizeof(commandID_));
	}
	void Write(XBuffer& out) const {
		out.write(&idxUserData_, sizeof(idxUserData_));
		out.write(&commandID_, sizeof(commandID_));
	};
	int idxUserData_;
	int commandID_;
};

struct netCommand4H_KickInCommand : NetCommandGeneral<NETCOM4H_KickInCommand>
{
	netCommand4H_KickInCommand(int _commandID, int _cooperariveIdx){
		commandID_=_commandID;
		teamIdx_=_cooperariveIdx;
	}
	netCommand4H_KickInCommand(XBuffer& in){
		in.read(&commandID_, sizeof(commandID_));
		in.read(&teamIdx_, sizeof(teamIdx_));
	}
	void Write(XBuffer& out) const {
		out.write(&commandID_, sizeof(commandID_));
		out.write(&teamIdx_, sizeof(teamIdx_));
	};
	int commandID_;
	int teamIdx_;
};

struct netCommand4C_DiscardUser : NetCommandGeneral<NETCOM4C_DiscardUser>
{
	enum { NOT_PLAYER_IDX=-1 };
	UNetID unid;
	netCommand4C_DiscardUser(const UNetID& _unid){
		unid=_unid;
	}
	netCommand4C_DiscardUser(XBuffer& in){
		in.read(&unid, sizeof(unid));
	}
	void Write(XBuffer& out) const {
		out.write(&unid, sizeof(unid));
	}
};



struct terEventControlServerTime : NetCommandGeneral<NETCOM4C_ServerTimeControl>
{
	float scale;

	terEventControlServerTime(float s){ scale = s; } // : NetCommandBase(NETCOM4C_ServerTimeControl)
	terEventControlServerTime(XBuffer& in) { in > scale; };
	void Write(XBuffer& out) const { out < scale; };
};


#endif
