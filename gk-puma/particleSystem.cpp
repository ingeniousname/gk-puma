#include "particleSystem.h"

#include <iterator>

#include "dxDevice.h"
#include "exceptions.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const D3D11_INPUT_ELEMENT_DESC ParticleVertex::Layout[4] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

const XMFLOAT3 ParticleSystem::EMITTER_DIR = XMFLOAT3(0.0f, 1.0f, 0.0f);
const float ParticleSystem::TIME_TO_LIVE = 1.0f;
const float ParticleSystem::EMISSION_RATE = 200.0f;
const float ParticleSystem::MAX_ANGLE = XM_PIDIV2 / 9.0f;
const float ParticleSystem::MIN_VELOCITY = 2.2f;
const float ParticleSystem::MAX_VELOCITY = 2.33f;
const float ParticleSystem::PARTICLE_SIZE = 0.08f;
const float ParticleSystem::PARTICLE_SCALE = 1.0f;
const float ParticleSystem::MIN_ANGLE_VEL = -XM_PI;
const float ParticleSystem::MAX_ANGLE_VEL = XM_PI;
const int ParticleSystem::MAX_PARTICLES = 5000;

ParticleSystem::ParticleSystem(DirectX::XMFLOAT3 emmiterPosition)
	: m_emitterPos(emmiterPosition), m_particlesToCreate(0.0f), m_random(random_device{}())
{ }

vector<ParticleVertex> ParticleSystem::Update(float dt, DirectX::XMFLOAT4 cameraPosition)
{
	size_t removeCount = 0;
	for (auto& p : m_particles)
	{
		UpdateParticle(p, dt);
		if (p.Vertex.Age >= TIME_TO_LIVE)
			++removeCount;
	}
	m_particles.erase(m_particles.begin(), m_particles.begin() + removeCount);

	m_particlesToCreate += dt * EMISSION_RATE;
	while (m_particlesToCreate >= 1.0f)
	{
		--m_particlesToCreate;
		if (m_particles.size() < MAX_PARTICLES)
			m_particles.push_back(RandomParticle());
	}
	return GetParticleVerts(cameraPosition);
}

XMFLOAT3 ParticleSystem::RandomVelocity()
{
	static uniform_real_distribution angleDist{ 0.f, XM_2PI };
	static uniform_real_distribution magnitudeDist{ 0.f, tan(MAX_ANGLE) };
	static uniform_real_distribution velDist{ MIN_VELOCITY, MAX_VELOCITY };
	float angle = angleDist(m_random);
	float magnitude = magnitudeDist(m_random);
	XMFLOAT3 v{ cos(angle)*magnitude, 1.0f, sin(angle)*magnitude };

	auto velocity = XMLoadFloat3(&v);
	auto len = velDist(m_random);
	velocity = len * XMVector3Normalize(velocity);
	XMStoreFloat3(&v, velocity);
	return v;
}

Particle ParticleSystem::RandomParticle()
{
	static uniform_real_distribution anglularVelDist{ MIN_ANGLE_VEL, MAX_ANGLE_VEL };
	Particle p;
	// TODO : 1.27 Setup initial particle properties
	p.Vertex.Pos = m_emitterPos;
	p.Vertex.Age = 0.f;
	p.Vertex.Angle = 0.f;
	p.Vertex.Size = PARTICLE_SIZE;

	p.Velocities.Velocity = RandomVelocity();
	p.Velocities.AngularVelocity = anglularVelDist(m_random);

	return p;
}

void ParticleSystem::UpdateParticle(Particle& p, float dt)
{
	// TODO : 1.28 Update particle properties
	p.Vertex.Age += dt;
	p.Vertex.Angle += p.Velocities.AngularVelocity * dt;
	p.Vertex.Pos.x += p.Velocities.Velocity.x * dt;
	p.Vertex.Pos.y += p.Velocities.Velocity.y * dt;
	p.Vertex.Pos.z += p.Velocities.Velocity.z * dt;

	p.Vertex.Size += PARTICLE_SCALE * PARTICLE_SIZE * dt;
}

vector<ParticleVertex> ParticleSystem::GetParticleVerts(DirectX::XMFLOAT4 cameraPosition)
{
	XMFLOAT4 cameraTarget(0.0f, 0.0f, 0.0f, 1.0f);

	vector<ParticleVertex> vertices;
	// TODO : 1.29 Copy particles' vertex data to a vector and sort them

	std::transform(m_particles.begin(), m_particles.end(), std::back_inserter(vertices), [](const auto& particle) {
		return particle.Vertex;
		});

	std::sort(vertices.begin(), vertices.end(), [&](const auto& v1, const auto& v2) {
		return XMVectorGetX(XMVector3Length(XMLoadFloat4(&cameraPosition) - XMLoadFloat3(&v1.Pos)))
			< XMVectorGetX(XMVector3Length(XMLoadFloat4(&cameraPosition) - XMLoadFloat3(&v2.Pos)));
		});

	return vertices;
}