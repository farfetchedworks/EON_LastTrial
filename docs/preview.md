# ENGINE 3D PREVIEW

## 1. Import animation/data in 3ds MAX
1.1 Add Cal3D Plugin to MAX from https://bit.ly/3geaWsu (if you can't access the engine repo, complain about it!)  
1.2 If you don't have the animation in MAX yet, it's time to import it

## 2. Prepare export
2.1 Save max file: it will be the exported animation name (the one which goes into the animatio list!)\
2.2 Don't use ":", "/" or any special character for the parent bone (this will be the skeleton name!!)\
2.3 Be sure there's only a parent mesh (Cal3D only exports one parent mesh with its children) and again don't use ":" or any special character\
2.4 Remove any material of the meshes different than a standard scanline material. Don't worry, it's only for exporting, we use	the good one for the render

## 3. Export
3.1 Open MAXScript tool and select Exporter\
3.2 Export Skeleton And Meshes\
3.3 Export Skeleton Animation\
3.4 If you did it right, check if the folder *bin/data/skeletons/"skeleton_name"* has been created and all the data exported is there\
3.5 Check for: animations as CAF, skeleton as CSF and meshes as CMF files

## 4. Create skeleton data for the engine
4.1 Open https://jxarco.github.io/FSMGraphEditor/  
4.2 At the menubar, in **Tools**, select Skeleton file exporter\
4.3 Fill the dialog with the skeleton name and the animations and meshes exported (without extensions!)\
4.4 Fill also the material stuff and add the textures that you need. Contact Alex if there's a texture that it's not supported there.\
4.5 The root folder is *bin/data/textures/* so write your path from there; for example **"eon/eonhood"** without extension (.dds is added automatically)\
4.6 Export both files to your skeleton folder at *bin/data/skeletons/"skeleton_name"*\

## 5. Configuring the engine preview
5.1 In *bin/data/scenes/3dviewer/*, open the **character_preview.json** file\
5.2 In that file, modify the **render** and the **armature** sections\
5.3 Add one pair <mesh, mat> for every render mesh you have in your model:\
> {  
>  "mesh": "data/skeletons/skeleton_name/mesh_name.mesh",  
>  "mat": "data/skeletons/skeleton_name/mat_name.mat"  
> },

5.4 Change the path of the armature to set your skeleton: **data/skeletons/"skeleton_name"/"skeleton_name".skeleton**

## 6. Testings your animations!
6.1 Press F1 to use the mouse cursor and click: **All entities** then **character_preview** and then **armature**\
6.2 There you can select the animation you want and apply it as a cycle or as an action\
