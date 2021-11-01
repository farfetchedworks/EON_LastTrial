# UPFMCV2021

Source code repo for the 2021 edition.

# External libs used:

https://github.com/ocornut/imgui
https://github.com/nlohmann/json
https://github.com/CedricGuillemet/ImGuizmo
https://github.com/microsoft/DirectXTK
https://developer.nvidia.com/gameworksdownload#?dn=dds-utilities-8-31

### PBR Links ###

https://www.cgbookcase.com/textures
https://freepbr.com/about-free-pbr/
http://dativ.at/lightprobes/index.html
https://github.com/derkreature/IBLBaker
https://github.com/GameTechDev/ASSAO

https://interplayoflight.wordpress.com/2013/12/30/readings-on-physically-based-rendering/
http://aras-p.info/texts/files/201403-GDC_UnityPhysicallyBasedShading_notes.pdf
https://www.fxguide.com/fxfeatured/game-environments-parta-remember-me-rendering/
https://blog.selfshadow.com/publications/s2014-shading-course/hoffman/s2014_pbs_physics_math_slides.pdf
https://learnopengl.com/PBR/Lighting

http://developer.amd.com/wordpress/media/2012/10/Oat-ScenePostprocessing.pdf
https://www.yumpu.com/en/document/view/18413313/real-time-3d-scene-post-processing-amd-developer-central/13
http://www.sunsetlakesoftware.com/2013/10/21/optimizing-gaussian-blurs-mobile-gpu
https://software.intel.com/content/www/us/en/develop/blogs/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms.html

### TASK ###

--- Pintar Particulas ---	
	simulation
	rendre <---
	Simulation en GPU????

--- Culling en GPU ------
    Subir a la gpu todos los datos de las mallas instanciables
	Hacer el culling en gpu
	Render por instancing los resultados

--- Decals por instancing...

--- IKs
--- Animacion Rigida
--- Culling por software
--- AABB's de los skinned

- Stencil
- Video player
- Reflection API

--- Instancing ---

Instanced Mesh  (la palmera)  (IMesh)
     *
Datos particulares de cada instancia (world, color)

--- How en DX ?? ------

3- Usar structured Buffers y hacer el instnacing a mano en el VS
   - Malla instanciada -> Tetera
   - Buffer de instancias, C++ tiene que saber el # de bytes per instancia (16 bytes, ...)
   - una llamada de DrawInstanced de DX
   - En el VS, usar la semantica INSTANCE_ID para acceder al buffer de instancias y leer lo que quiera

   - Crear una malla instancia CInstancedMesh : public CMesh
			Buffer de datos
			Malla instancia




2- Usar instancing por HW
Vertex Declaration Nueva
Se crean dos streamings de vertex buffers
	1o -> Malla instanciada
	2o -> Malla de las instancias. Cada vertice representa una instancia
	      Marco este Stream 
Usar una llamada de DrawInstanced de Dx


1- Instancing sin el driver (**very old school***)
IMesh | IMesh | IMesh | IMesh | IMesh | IMeshN
PosNUvTan,InstanceID

Cte Buffer
World0 World1 World2 World2 ... WorldN

VS -> 



- Glow/Bloom
	- Blur 1D
	- Blur 2D
	- Cadena de Blurs
	- Combinar resultados
- Depth Of Field

- Decals 3D
	- Pintar una cubo deformado en el mundo
	- Pixel mundo -> Coordenadas locales de ese cubo XZ
		- Accede a una textura 2D (mancha de sangre)
		- Modificare el gbuffer (blending en el albedo)

- Instancing 

- Cel Shading Proto
+ AO 


- 



 + NormalMap
	+ Calculo Tangent space
	+ Uso en un shader
	+ Normal map para skin meshes

 - render debug de meshes en wireframe
 - Inverse Kinematics
 - Problem Custom Attributes
	- Create a mesh, add custom attr, copy obj, the copied obj
	  does have the custom attr, but the values/id are not correct
	  Bind to the copy event?
 + Report Callstack errors
 + Techs/pipelines puedan 'activar' texturas al activar
 + CPU Culling...
 + Named Enums...

 - Deferred shading
	- 1 RT -> 4 RT 
	- AO	
		- Propuesta
		- Intel
	- Lights en un RT separado (acumula luces)
		- Ambient pass -> PBR
			- CubeMaps para el env
		- Direccionales
		- Puntuales sin shadows
		- Puntuales con shadows
			- Cubemap shadowmap
	- Linear -> Gamma space
	- CellShading
	- HDR -> LDR
	- Postprocesados
		- LUT
		- Deteccion bordes
		- Glows
		- ...
		- FXAA
		- Distorsiones

- Decals 3D
- Transparent
- Particles
- Instancing
- CS
	- Particles
	- Frustum Culling
		- Lod, instancing
- Stencil
- Video player
- Reflection API


+ Render Blend/Config/
+ Marcas de scope de render/cpu/debug gpu
+ Profiling
+ Render... 
+ Performance metrics



+ CompAttachedToBone
	Skin
	BoneName
	Delta
	Updates Transform 
+ Support for autogeneration of the technique associated to the Skin
- Exportador de Cal3D en Max
	+ Para exportar el Skeleton 
		- Poner el skeleto en pose T
		- Seleccionar el bone root
		- File -> Export -> Extension CSF
		- Por ejemplo maw.csf
		- Seleccionar la lista de bones (todos)
	 -Para Exportar la mesh
		- Poner el esqueleto en pose T
		- Seleccionar la mesh que quiero exportar
		- File -> Export -> Extension CMF
		- Por ejemplo maw.cmf
		- Te pide el fichero .csf al que va a ir asociado
			- Excoder el maw.csf anterior
		- Escoger max bones 4 
		- 0.01 de min influencia esta ok
		- ...
		- No queremos LOD
		- No queremos Spring bones
	- Para exportar cada clip de animacion
		- El esquelete NO tiene que estar en pose T
		- Seleccionar el root del esqueleto
		- File -> Export -> Extension CAF
		- Por ejemplo idle.caf
		- Te pide el fichero .csf al que va a ir asociado
			- Excoder el maw.csf anterior
		- Seleccionar que bones queremos que esta animacion afecte.
			- Un ciclo todos
			- Una accion parcial por los bones.
		- Escoger el rango de frames del max
		- 



- Render Skinned Meshes
	Input:
		Cal .cmf -> Pos, UV, Tan, Skinning
	Output
		.mesh -> Vertex Format que incluya informacion de skinning
			4 x BoneId
			4 x Weight
		Una vez al arrancar, 
	Vertex Shader que use esa informacion
		Cada Bone -> Matriz World
		1 Skel de 80 Bones -> 80 Matrices
		Cada vertex
		Vfinal = Vpos * M0 * W0 
		       + Vpos * M1 * w1 
			   + VPos * M2 * w2 
			   + VPos * M3 * w3 = vPos * ( M0 * w0 + M1 * w1 + M2 * w2 + M3 * w3 )
	Cada CompSkel -> Tiene que subir a la GPU las 80 matrices como
	     cte buffer
	PS no cambia

- Skinned meshes...
	- Cal3d c++, antigua
	  Esta bien estructura, Naming
	  En Debug es lenta, std::vector, ...
	  C++, skinning CPU no es viable, GPU, compute shader
- Animacion Mallas
	- Animacion Rigida
	- Animacion Skinning/Skeletal
		- CoreModel
			- 1 Skeleton
				- N bones puestos en jerarquia 1 padre / K Hijos
			- N Mallas 'pesadas/skinned' al skeleton
				- Cada vertices tiene:
					- Pos, N, us, Tan,..
					- 4 bonesId's (identificador de bone)
						- 8 bits/bone -> 256 bones distintos
						- x4 bones -> 32bits
					- 4 weights  (en que porcentaje sigo a ese 
	bone)
						- Suma de weights = 1
						- float -> 32 bits, 8 bits 0...255 
							x4 -> 32 bits
				- Si un vertice solo usa 1 bone como influencia
					Pues le pones 1, 0, 0, 0 a los weights
			- M Clips de Animacion
				- Cada clip de animacion dice:
					- Como se mueven unos cuantos bones del skeleton
					- Respecto a su padre, se guardan es espacio local
					- Durante un tiempo
		- Instancias de esqueleto:
			- Puntero a los datos CoreModel
			- Skeleton
				- N Bones 
			- Mallas que estan asociadas a esa instancia... (no lo usaremos)
			- Mixer
				- Guarda que animaciones se estan reproduciendo ahora mismo en el esqueleto
					- Puedo estar corriendo y saludando a la vez
					- o Solo corriendo
					- O corriendo al 30% y caminando al 70%
					- Todas las animaciones tienen un currentTime
					- Todas las animaciones tienen un Weight
					- Todas las animaciones entran y salen con blendTime
				- Hay dos tipos de animaciones
					- Ciclos
						- Hacen loop automaticamente
						- Para quitarlas hay que quitarlas manualmente
					- Acciones
						- Entran Manualmente y salen automaticamente despues del acabar
				- Mezclas los ciclos en funcion de su peso
				- Mezclar las acciones en funcion de su peso
				- El resultado final es lo que digan las acciones, si la accion no dijo 'nada'
				  se ve lo que marcaron los ciclos
		- Tipos de ficheros
			.csf -> Cal Skeleton File
			.cmf -> Cal Mesh file
			.caf -> Cal Animation file







- Fixes to exporter
	- Meshes in m
	- Export colliders
	- Material diffuse texture is required
- Multimaterial 1 Mesh -> N Materiales
	- Sort faces by material
	- Write ranges
	- Write materials names?
	- Read
	- Render by material id
- AABB 
	- Compute in the exporter as part of the header
	- Comp AABB absoluto y local
	- Culling...


+ Lector de colliders
+ Load convex mesh for collision

+ Cache de los json. Para que cada instancia un prefab
  NO quiero parsear el prefab otra vez.
+ Prefabs
	- Miniscene -> mismo formato de escena 
		array de entitdades
		- componentes
	- Instancias un prefab/scene
		- aparezca la escana pero transformada en la posicion de instanciacion
		- referencias entre entidades de un prefab
	+ Confirmar que no se llama al start 2 veces cuando una entidad 
	  sale de un prefab. Case del physics

+ Tags
+ Instanciar en un punto del mapa
+ Jerarquia entre entidades


- Padre/Hijos
	- Si padre muere, los hijos tambien
	- Se padre se hace invisible, tambien los hijos???
	- Se padre se hace 'disabled', tambien los hijos???
	- Se padre recibe un msg, hace un forward a sus hijos???
	- Me interesa el getCompInHierarchy....
	- GetAllChildrenWithComponent...

- Puede/Quiero recargar un prefab????


- Enable/Disabled????

- Meto un TCompEnabled ( bool true / false )
	RunTime????
		Todos los managers de compoenents tendria que ir a mirar 
		si el compenabled des sa NO

	Managers tiene un array lineal de components
	Imaginemos que tenemos un manager con 20 comps, 12 Enabled y 8 Disabled
	Ordenemoslos por primeros los enabled y luegos los disabled
	Y le digo al manager que en vez de hacer update de 20, lleve la cuenta
	de cuantos 'enabled' tiene, y haga un bucle para los primeros 12

	Coste en runtime 0
	Coste de cambiar el flag enabled true/false
		Pateate por todos los componentes que tiene un update
		y reordenalos para confirmar ese criterior
	El cambio NO seria inmediato, sino a final de frame

	Problema con el TCompRender q no tiene update

- Opcion 2
	Los componentes que necesitan el enable/disabled tiene un flag
	bool private
	Hay un msg que setEnable, el componente reacciona y punto
	en el update hacer un if(!enabled) return;




+ Exportador de colisiones
	+ Exportar el comp_collider
	+ Render de la malla de colision (como trimesh)
	+ Cache cooked mesh from physics


############################



	// Enviado por los miembros del squad al brain cuando ven al player
	// Tambien se puede usar por el Brain para informar a los miembros de la ultima posicion
- MsgPlayerSeen {
	VEC3 where;
	};

	// Enviado por las entidades enemigas a su BrainSquad cuando 
	// empiezan
- MsgRegisterInSquad {
	CHandle new_member;
	};

- CompIASquadBrain {
	// Array of CHandles 
	std::vector<CHandle> my_squad;
	bool in_alarm = false
	
	- Recv MsgPlayerSeen {
		- If not in alarm -> send MsgPlayerSeen to all members of my squad
	}

- CompIASquadMember {
	- On Start send a msg to your brain to register youself
	- If not in alarm
		Use the transform to check if see the player in our cone vision
		If YES -> Send msg MsgPlayerSeen to the SquadBrain entity about the location of the player.
	- If the Brain send us a MsgPlayerSeen, enter in alarm state
	  and orient to the target position 
}

// en el json
  "entity" : {
		"name" : "red"
		"brain" : {},
		}
  "entity" : {
		"name" : "member1_red"
		"squad" : { "brain":"red"},
		}...
  "entity" : {
		"name" : "member1_red"
		"squad" : { "brain":"red"},
		}

"Repeating for blue, and some members for blue could provide two
separate squads."

Notas:
Obtener handle to this:   CHandle(this)
Obtener entity by name:   getEntityByName('name') 
Crear primero los brain's y luego los members...
Los members tienen que leer y guardar el nombre del brain hasta el start.

### END OF TASK ###

- json as Resource to avoid parsing json everytime we required them
- InstantiatePrefabAt...
	- Parsing an entity requires creating the entities at the spawn transform

+ Time -> real time 

+ Debug Components
	+ CompTransform render the axis
	- CompName render the name of the entity
	- CompIA renders the vision cones

+ axis/line.... -> resources
+ Facilitar que un componente acceda a componentes hermanos

+ Resource Material
+ CompRender
	- Mesh
	- Material
+ RenderManager

-- /////////////////////// 

- Clone repo
- Add TCompAimTo {
	std::string target;
	update() {
	  target_name -> CHandle -> CEntity ( solo la primera vez )
	  Entity -> Su Transform
	  Access to Nuesrta transform
		this es un puntero a TCompAimTo
		CHandle(this)   <-- Handle q me representa a mi(this)
		CEntity* my_entity = CHandle(this).getOwner(); <-- Handle a la Entity propietaria
		TCompTransform* c_transform = my_entity->get<TCompTransform>();

	  Update Nuestra transform
	}
}
- Que alguna entidad de la escena lo use

- Falta activar el module_entities en el update del engine:
	en data/modules.json 
  "update": [
    "input_1",
    "entities",					<< -----
    "splash_screen",
    "main_menu",
    "john"
  ],
- Activar el TCompAimTo como component a actualizarse en 'data/components.json'
  "update": [
    "aim_to"
  ],


# TODO

- HotReload resources
- line/grid must be owned by the ResourcesManager
- Remove the ugly cast to const IResource**

+ ImGui para esos resources
+ samLinear is not defined
+ Remove hardcoded 0 from texture activate
+ Cargar y usar las texturas
+ Texturas como Resource
+ Color.h

+ Resource
+ ResourceManager 
+ HotReload de ficheros

+ Exportador de MaxScript de mallas
	- Formato propio. Basado RIFF 
		IDCabecera | TamanyoBloque | DatosBloque
		...
+ Importador de mallas desde disco

+ Test Front/Back/Left/Right/Angle
+ DrawLine

+ functions to rad/deg conversion
+ CTransform
+ ZBuffer


== Quaternions ==
- rotation de x grados sobre un eje x,y,z
   x = rotation_axis.x * sin( angle * 0.5 )
   y = rotation_axis.y * sin( angle * 0.5 )
   z = rotation_axis.z * sin( angle * 0.5 )
   w = cos ( angle * 0.5 )
- son 4 floats
- operaciones q * punto 
  concatenar rotaciones es multiplicar quaterniones q = q1*q2 
  es muy facil invertir un quaternion -x,-y,-z,w
  no es comunicastiva
  normalizar 
