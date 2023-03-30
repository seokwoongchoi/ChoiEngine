#pragma once
class Transforms
{
public:
	explicit Transforms(ID3D11Device* device,class Animator* animator);
	~Transforms();
private:

	Transforms(const Transforms &) = delete;
	Transforms & operator= (const Transforms &) = delete;

public:
	void Update(const uint & actorIndex, const uint & index);
	
public:
	void ReadBehaviorTree(BinaryReader * r, const uint & actorIndex);
private:
	
	vector<class BehaviorTree*>behaviorTrees;
	class Animator* animator;
	
	 Vector3 position;
	 Quaternion quat;
	 Vector3 scale;
	 Matrix inst;

	 int btIndex[2];
};

