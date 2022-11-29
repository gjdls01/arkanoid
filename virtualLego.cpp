////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ¹ÚÃ¢Çö Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

IDirect3DDevice9* Device = NULL;

// window size
const int Width  = 1024;
const int Height = 768;

const float spherePos[54][2] = {
	{-2.5,2},{0.5,0.5},{0.5,-0.5},{-0.5,0.5},{-0.5,-0.5},{0,0.5},{0,-0.5},{-0.5,0},{0.5,0},
	{2.5,2},{2.5,-2},{2,2.5},{2,-2.5},{0,2.5},{0,-2.5},{-2,2.5},{-2,-2.5},{2.5,0},
	{3,1},{3,-1},{-3,1},{-2.5,0},{-3,-1},{-1,0.5},{-1,-0.5},{1.5,1.5},{1.5,-1.5},
	{-1.5,1.5},{-1.5,-1.5},{0,1.5},{0,-1.5},{-1.5,0},{1.5,0},{1,1.5},{1,-1.5},{-1,1.5},
	{-1,-1.5},{1.5,1},{1.5,-1},{-1.5,1},{-1.5,-1},{0.5,1.5},{0.5,-1.5},{-0.5,1.5},{-0.5,-1.5},
	{1.5,0.5},{1.5,-0.5},{-1.5,0.5},{-1.5,-0.5},{-2.5,2.5},{-2.5,-2.5},{2.5,2.5},{2.5,-2.5},{-2.5,-2}
}; 

bool noGame = true;

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private :
	float center_x, center_y, center_z; //position of sphere: x,y,z
    float m_radius; //radius of sphere
	float m_velocity_x; //velocity of sphere to the direction of x
	float m_velocity_z; //velocity of sphere to the direction of x
	bool bottomBall = false; //indicate the ball is moving ball (as it moves at the bottom of the plane, it is named as bottomball)
public:
    CSphere(void)
	{
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
        m_pSphereMesh = NULL;
    }
    ~CSphere(void) {}

public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;
		
        m_mtrl.Ambient  = color;
        m_mtrl.Diffuse  = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power    = 5.0f;
		
        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }
	
    void destroy(void){
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld){
        if (NULL == pDevice) return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
    }
	
    bool hasIntersected(CSphere& ball) {
		D3DXVECTOR3 diff = this->getCenter() - ball.getCenter();
		float dist = sqrt(diff.x * diff.x + diff.z * diff.z);
		if (this->getRadius()+ball.getRadius() < dist) return false;
		else return true;
	}
	
	void hitBy(CSphere& ball){ //if ball is g_dirS, it destroys the targets balls. if ball is g_movS, ball changes the direction 
		if (hasIntersected(ball)) { //distance = velocity * time
			D3DXVECTOR3 diff = this->getCenter() - ball.getCenter();
			float diff_x = diff.x;
			float diff_z = diff.z;
			float dist_xz = sqrt(diff_x * diff_x + diff_z * diff_z);

			float velocity_x = ball.getVelocity_X();
			float velocity_z = ball.getVelocity_Z();
			float velocity_xz = sqrt(velocity_x*velocity_x + velocity_z * velocity_z);

			double velocity_x2 = velocity_xz / dist_xz * diff_x;
			double velocity_z2 = velocity_xz / dist_xz * diff_z;

			ball.setPower(-velocity_x2, -velocity_z2); //to make the change on the direction, put 'minus'
			//except for the moving ball, there are target spheres. It is make them disapear on the screen
			if (!this->isBottomBall()) { this->setCenter(-7,-7,-7); } 
		}
	}

	void ballUpdate(float timeDiff) {
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		if(vx > 0.01 || vz > 0.01){
			float tX = cord.x + TIME_SCALE*timeDiff*m_velocity_x;
			float tZ = cord.z + TIME_SCALE*timeDiff*m_velocity_z;
			
			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0,0);}
	}

	double getVelocity_X() { return this->m_velocity_x;	}
	double getVelocity_Z() { return this->m_velocity_z; }

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z){
		D3DXMATRIX m;
		center_x=x;	center_y=y;	center_z=z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}
	
	float getRadius(void)  const { return (float)(M_RADIUS);  }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

    D3DXVECTOR3 getCenter(void) const{
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }
	
	void setBottomBall() { bottomBall = true; }
	bool isBottomBall() { return bottomBall; } 
	
private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh*              m_pSphereMesh;
	
};

// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {
private:
    float m_x;
	float m_z;
	float m_width;
    float m_depth;
	float m_height;
public:
    CWall(void){
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE){
        if (NULL == pDevice) return false;
		
        m_mtrl.Ambient  = color;
        m_mtrl.Diffuse  = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power    = 5.0f;
		
        m_width = iwidth;
        m_depth = idepth;
		
        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL))) return false;
        return true;
    }

    void destroy(void){
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }

    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice) return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
    }
	
	bool hasIntersected(CSphere& ball) {
		float ballx = ball.getCenter().x;
		float ballz = ball.getCenter().z;;
		float ballr = ball.getRadius();

		float top = this->getPosition().x - this->getWidth() * 0.5 - ballr;
		float down = this->getPosition().x + this->getWidth() * 0.5 + ballr;
		float left = this->getPosition().z - this->getDepth() * 0.5 - ballr;
		float right = this->getPosition().z + this->getDepth() * 0.5 + ballr;

		if ((top <= ballx && ballx <= down) && (left <= ballz && ballz <= right)) return true;
		else return false;
	}

	void hitBy(CSphere& ball) 
	{
		if (hasIntersected(ball)) {
			float ballx = ball.getCenter().x;
			float ballz = ball.getCenter().z;
			float ballr = ball.getRadius();

			float top = this->getPosition().x - this->getWidth() * 0.5;
			float down = this->getPosition().x + this->getWidth() * 0.5;
			float left = this->getPosition().z - this->getDepth() * 0.5;
			float right = this->getPosition().z + this->getDepth() * 0.5;

			if ((top <= ballx && ballx <= down) && !(left <= ballz && ballz <= right)) {
				ball.setPower(ball.getVelocity_X(), -ball.getVelocity_Z()); 
				if (top - ballr <= ballz && ballz <= this->getPosition().z) {
					ballz = left - ballr;
				}
				else ballz = right + ballr;
			}
			if (!(top <= ballx && ballx <= down) && (left <= ballz && ballz <= right)) {
				ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
				if (left - ballr <= ballx && ballx <= this->getPosition().x) {
					ballx = top - ballr;
				}
				else ballx = down + ballr;
			}
			ball.setCenter(ballx, ball.getCenter().y, ballz);
		}
	}    
	
	void setPosition(float x, float y, float z){
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	D3DXVECTOR3 getPosition(void)const {
		D3DXVECTOR3 pos(m_x, 0, m_z);
		return pos;
	}
	
    float getHeight(void) const { return M_HEIGHT; }
	float getWidth(void) const { return m_width; }
	float getDepth(void) const { return m_depth; }

	D3DXVECTOR3 getCenter(void) const {
		D3DXVECTOR3 org(m_x, 0, m_z);
		return org;
	}
	
	
private :
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	
	D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh*              m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void){
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f){
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;
		
        m_bound._center = lit.Position;
        m_bound._radius = radius;
		
        m_lit.Type          = lit.Type;
        m_lit.Diffuse       = lit.Diffuse;
        m_lit.Specular      = lit.Specular;
        m_lit.Ambient       = lit.Ambient;
        m_lit.Position      = lit.Position;
        m_lit.Direction     = lit.Direction;
        m_lit.Range         = lit.Range;
        m_lit.Falloff       = lit.Falloff;
        m_lit.Attenuation0  = lit.Attenuation0;
        m_lit.Attenuation1  = lit.Attenuation1;
        m_lit.Attenuation2  = lit.Attenuation2;
        m_lit.Theta         = lit.Theta;
        m_lit.Phi           = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld){
        if (NULL == pDevice)
            return false;
		
        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;
		
        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice){
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh*          m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[3];
CSphere	g_sphere[54]; //yellow balls (target balls that shoudl be destroyed)
CSphere	g_dirS; //red ball (dirS has meaning 'direction Sphere', which make a change on the direction after the intersection with other balls)
CSphere g_movS; //white ball (movS has meaning 'moving Sphere', which moves at the bottom of plane, not the red ball to fall down at the bottom of the plane.
CLight	g_light;

double g_camera_pos[3] = {0.0, 5.0, -8.0};

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup(){
	int i;
	
    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);
		
	// create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 9, 0.03f, 6, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);
	
	// create walls and set the position
	if (false == g_legowall[0].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, 0.12f, 3.06f);
	if (false == g_legowall[1].create(Device, -1, -1, 9, 0.3f, 0.12f, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(0.0f, 0.12f, -3.06f);
	if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 6.24f, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(-4.56f, 0.12f, 0.0f);

	// create  balls and set the position 
	for (i=0;i<54;i++) {//54: number of target balls
		if (false == g_sphere[i].create(Device, d3d::YELLOW)) return false; 
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS , spherePos[i][1]);
		g_sphere[i].setPower(0,0); //target balls don't have any velocity (dont' move)
	}
	
    if (false == g_movS.create(Device, d3d::WHITE)) return false;
	g_movS.setCenter(g_legoPlane.getWidth()*0.5, 0.5, .0f);
	g_movS.setBottomBall(); //white ball is at the bottom of the plane.

	if (false == g_dirS.create(Device, d3d::RED)) return false;
	g_dirS.setCenter(g_legoPlane.getWidth() * 0.5, 0.5, .0f);

	// light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type         = D3DLIGHT_POINT;
    lit.Diffuse      = d3d::WHITE; 
	lit.Specular     = d3d::WHITE * 0.9f;
    lit.Ambient      = d3d::WHITE * 0.9f;
    lit.Position     = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range        = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;
	
	// Position and aim the camera.
	D3DXVECTOR3 pos(10.0f, 10.0f, 0.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);
	
	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);
	
    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	
	g_light.setLight(Device, g_mWorld);
	return true;
}

void Cleanup(void){
    g_legoPlane.destroy();
	for(int i = 0 ; i < 3; i++) {
		g_legowall[i].destroy();
	}
    destroyAllLegoBlock();
    g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i=0;

	if( Device )
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		// update the position of each ball. during update, check whether each ball hit by walls.
		for (i = 0; i < 54; i++) { 
			g_sphere[i].ballUpdate(timeDelta);
			g_sphere[i].hitBy(g_dirS);
		}

		g_dirS.ballUpdate(timeDelta);
		g_movS.ballUpdate(timeDelta);

		for (i = 0; i < 3; i++) {
				g_legowall[i].hitBy(g_dirS);
				g_legowall[i].hitBy(g_movS);
		}

		g_movS.hitBy(g_dirS);
		
		if (noGame) { g_dirS.setCenter(g_movS.getCenter().x - 2 * M_RADIUS, g_movS.getCenter().y, g_movS.getCenter().z); }
		if (g_legoPlane.getPosition().x + g_legoPlane.getWidth() * 0.5 <= g_dirS.getCenter().x) { //when red ball is out of the plane
			noGame = true;
			g_dirS.setPower(0, 0);
			g_dirS.setCenter(g_movS.getCenter().x - 2 * M_RADIUS, g_movS.getCenter().y, g_movS.getCenter().z);

			for (i = 0;i < 54;i++) {
				g_sphere[i].setCenter(spherePos[i][0], M_RADIUS, spherePos[i][1]);
				g_sphere[i].setPower(0, 0);
			};
		}

		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i=0;i<3;i++) 	{
			g_legowall[i].draw(Device, g_mWorld);
		}
		for (i = 0;i < 54;i++) {
			g_sphere[i].draw(Device, g_mWorld);
		}
		g_dirS.draw(Device, g_mWorld);
		g_movS.draw(Device, g_mWorld);
        g_light.draw(Device);
		
		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture( 0, NULL );
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;
	
	switch (msg) {
	case WM_DESTROY: 
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN: 
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;
		case VK_SPACE:
			if (noGame) {
				noGame = false; //when we press space, the game starts
			}
			g_dirS.setPower(-3.0f, 0.0);
		}
		break;
	}
	case WM_MOUSEMOVE: 
	{
		int new_x = LOWORD(lParam);
		int new_y = HIWORD(lParam);
		float dx;
		float dy;

		float left = g_legowall[1].getPosition().z + g_legowall[1].getDepth() * 0.5 + g_movS.getRadius();
		float right = g_legowall[0].getPosition().z - g_legowall[0].getDepth() * 0.5 - g_movS.getRadius();

		if (LOWORD(wParam) & MK_LBUTTON) {

			D3DXVECTOR3 ballCenters = g_movS.getCenter();
			dx = old_x - new_x;
			dy = old_y - new_y;

			float z0 = ballCenters.z + dx * (-0.007f);

			if (z0 < left) {
				z0 = left;
			}
			if (z0 > right) {
				z0 = right;
			}

			g_movS.setCenter(ballCenters.x, ballCenters.y, z0);
			old_x = new_x;
			old_y = new_y;

			move = WORLD_MOVE;
		}
		break;
	}
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));
	
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device)){
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}
	
	if(!Setup()){
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}
	
	d3d::EnterMsgLoop( Display );
	
	Cleanup();
	
	Device->Release();
	
	return 0;
}