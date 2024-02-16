#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"

#if defined(_MSC_VER)
    #include <io.h>
#endif
#include <stdio.h>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <vector>


// double resolution_difference(const ModelConfiguration& config1, const ModelConfiguration& config2) {
//     double diffX = std::abs(static_cast<double>(config1.resolutionX - config2.resolutionX));
//     double diffY = std::abs(static_cast<double>(config1.resolutionY - config2.resolutionY));
//     double diffZ = std::abs(static_cast<double>(config1.resolutionZ - config2.resolutionZ));
//     return diffX + diffY + diffZ;
// }


// std::pair<int, int> select_parents(const ModelConfiguration* population, int populationSize) {
//     // Calcula la puntuación de similitud entre cada par de modelos
//     std::vector<double> similarityScores(populationSize * (populationSize - 1) / 2);
//     int scoreIndex = 0;
//     for (int i = 0; i < populationSize - 1; ++i) {
//         for (int j = i + 1; j < populationSize; ++j) {
//             similarityScores[scoreIndex++] = resolution_difference(population[i], population[j]);
//         }
//     }

//     // Calcula las probabilidades ponderadas de selección para cada modelo
//     double totalScore = std::accumulate(similarityScores.begin(), similarityScores.end(), 0.0);
//     std::vector<double> selectionProbabilities(populationSize);
//     for (int i = 0; i < populationSize; ++i) {
//         double modelScore = 0.0;
//         for (int j = 0; j < populationSize; ++j) {
//             if (i != j) {
//                 modelScore += totalScore / similarityScores[j * (populationSize - 1) + (i < j ? i : i - 1)];
//             }
//         }
//         selectionProbabilities[i] = modelScore;
//     }

//     // Selecciona dos padres basados en las probabilidades ponderadas
//     int parent1 = 0, parent2 = 0;
//     double maxProb1 = 0.0, maxProb2 = 0.0;
//     for (int i = 0; i < populationSize; ++i) {
//         double randomProb = static_cast<double>(rand()) / RAND_MAX;
//         if (randomProb > maxProb1) {
//             maxProb1 = randomProb;
//             parent1 = i;
//         }
//         if (randomProb > maxProb2 && i != parent1) {
//             maxProb2 = randomProb;
//             parent2 = i;
//         }
//     }

//     return std::make_pair(parent1, parent2);
// }

const ogt_vox_scene* load_vox_scene(const char* filename, uint32_t scene_read_flags = 0)
{
    // open the file
#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE * fp;
    if (0 != fopen_s(&fp, filename, "rb"))
        fp = 0;
#else
    FILE * fp = fopen(filename, "rb");
#endif
    if (!fp)
        return NULL;

    // get the buffer size which matches the size of the file
    fseek(fp, 0, SEEK_END);
    uint32_t buffer_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // load the file into a memory buffer
    uint8_t * buffer = new uint8_t[buffer_size];
    fread(buffer, buffer_size, 1, fp);
    fclose(fp);

    // construct the scene from the buffer
    const ogt_vox_scene * scene = ogt_vox_read_scene_with_flags(buffer, buffer_size, scene_read_flags);

    // the buffer can be safely deleted once the scene is instantiated.
    delete[] buffer;

    return scene;
}

const ogt_vox_scene* load_vox_scene_with_groups(const char* filename)
{
    return load_vox_scene(filename, k_read_scene_flags_groups);
}

// a helper function to save a magica voxel scene to disk.
void save_vox_scene(const char* pcFilename, const ogt_vox_scene* scene) 
{
    // save the scene back out. 
    uint32_t buffersize = 0;
    uint8_t* buffer = ogt_vox_write_scene(scene, &buffersize);
    if (!buffer)
        return;

    // open the file for write
#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE * fp;
    if (0 != fopen_s(&fp, pcFilename, "wb"))
        fp = 0;
#else
    FILE * fp = fopen(pcFilename, "wb");
#endif
    if (!fp) {
        ogt_vox_free(buffer);
        return;
    }

    fwrite(buffer, buffersize, 1, fp);
    fclose(fp);
    ogt_vox_free(buffer);
}

// this example just counts the number of solid voxels in this model, but an importer 
// would probably do something like convert the model into a triangle mesh.
uint32_t count_solid_voxels_in_model(const ogt_vox_model* model)
{
    uint32_t solid_voxel_count = 0;
    uint32_t voxel_index = 0;
    for (uint32_t z = 0; z < model->size_z; z++) {
        for (uint32_t y = 0; y < model->size_y; y++) {
            for (uint32_t x = 0; x < model->size_x; x++, voxel_index++) {
                // if color index == 0, this voxel is empty, otherwise it is solid.
                uint32_t color_index = model->voxel_data[voxel_index];
                bool is_voxel_solid = (color_index != 0);
                // add to our accumulator
                solid_voxel_count += (is_voxel_solid ? 1 : 0);
            }
        }
    }
    return solid_voxel_count;
}


//APARTIR DE ACA IMPLEMENTACION PROPIA
struct ModelConfiguration {
    uint32_t resolutionX;
    uint32_t resolutionY;
    uint32_t resolutionZ;
    int material;
    // Otros parámetros que podrías querer ajustar
    // ...

    // Constructor para inicializar aleatoriamente las configuraciones
    ModelConfiguration() {
        resolutionX = rand() % 25 + 1;
        resolutionY = rand() % 25 + 1;
        resolutionZ = rand() % 25 + 1;
        material = rand() % 6;
    }
    ModelConfiguration(const ogt_vox_model* model) {
        resolutionX = model->size_x;
        resolutionY = model->size_y;
        resolutionZ = model->size_z;
        material = 2; // Puedes establecer un valor predeterminado para el material si lo deseas
    }
};

ogt_vox_model apply_configuration(const ModelConfiguration& configuration, const ogt_vox_model* originalModel) {
    ogt_vox_model modifiedModel = *originalModel;

    // Aplicar la nueva resolución
    modifiedModel.size_x = configuration.resolutionX;
    modifiedModel.size_y = configuration.resolutionY;
    modifiedModel.size_z = configuration.resolutionZ;

    // Otras modificaciones según tus necesidades
    // ...

    return modifiedModel;
}

// Función de evaluación que mide la calidad de una configuración del modelo
double evaluate_model(const ModelConfiguration& configuration, const ogt_vox_model* model) {
    // Ejemplo simple: Utilizar la resolución como medida de calidad
    double resolutionQuality = static_cast<double>(configuration.resolutionX * configuration.resolutionY * configuration.resolutionZ);

    // Otros factores que podrías querer considerar en la evaluación
    // ...

    return resolutionQuality;
}

double evaluate_model(const ModelConfiguration& configuration, const ogt_vox_model* originalModel, const ogt_vox_model* modifiedModel) {
    // Calcular la diferencia en términos de colores de voxels entre el modelo original y el modificado
    double colorDifference = 0.0;
    for (uint32_t i = 0; i < originalModel->size_x * originalModel->size_y * originalModel->size_z; i++) {
        uint8_t originalColor = originalModel->voxel_data[i];
        uint8_t modifiedColor = modifiedModel->voxel_data[i];
        colorDifference += std::abs(originalColor - modifiedColor);
    }

    // Calcular la reducción de peso en términos de resolución
    double originalWeight = static_cast<double>(originalModel->size_x * originalModel->size_y * originalModel->size_z);
    double modifiedWeight = static_cast<double>(configuration.resolutionX * configuration.resolutionY * configuration.resolutionZ);
    double weightReduction = originalWeight - modifiedWeight;

    // Calcular la calidad visual (puedes ajustar estos pesos según tu preferencia)
    double visualQuality = 1.0 - (colorDifference / (255.0 * originalModel->size_x * originalModel->size_y * originalModel->size_z));

    // Combinar la reducción de peso y la calidad visual para obtener la evaluación total
    double totalFitness = 0.7 * visualQuality + 0.3 * weightReduction;

    return totalFitness;
}

// Función para aplicar una mutación a una configuración del modelo
void mutate_configuration(ModelConfiguration& configuration) {
    // Ejemplo simple: Cambiar aleatoriamente la resolución
    configuration.resolutionX = std::max(1, static_cast<int>(configuration.resolutionX * 0.8));
    configuration.resolutionY = std::max(1, static_cast<int>(configuration.resolutionY * 0.8));
    configuration.resolutionZ = std::max(1, static_cast<int>(configuration.resolutionZ * 0.8));

    // int randomValue = rand() % 2;

    // // Si el resultado del operador módulo 2 es 0, establecer la propiedad material en 0
    // if (randomValue == 0) {
    //     configuration.material = 0; // Modificar según sea necesario si el valor predeterminado no es 0
    // }
    // Otros posibles ajustes/mutaciones
    // ...
}

ModelConfiguration crossover(const ModelConfiguration& parent1, const ModelConfiguration& parent2) {
    ModelConfiguration child;

    // Ejemplo simple: Cruce de un punto
    int crossoverPointX = rand() % 2;
    int crossoverPointY = rand() % 2;
    int crossoverPointZ = rand() % 2;

    child.resolutionX = (crossoverPointX == 0) ? parent1.resolutionX : parent2.resolutionX;
    child.resolutionY = (crossoverPointY == 0) ? parent1.resolutionY : parent2.resolutionY;
    child.resolutionZ = (crossoverPointZ == 0) ? parent1.resolutionZ : parent2.resolutionZ;

    return child;
}

// Función principal del algoritmo genético
void genetic_algorithm(const ogt_vox_model* model, const ogt_vox_model* originalmodel) {
    const int populationSize = 50;
    const int generations = 100;

    // Inicializar población
    ModelConfiguration population[populationSize];
    ModelConfiguration bestConfiguration;

        // Tomar el modelo original como primer individuo de la población
    population[0] = ModelConfiguration(originalmodel);

    // Iterar para generar el resto de la población
    for (int i = 1; i < populationSize; i++) {
        // Generar una configuración aleatoria dentro del rango del 20% hacia abajo
        ModelConfiguration randomConfig;
        randomConfig.resolutionX = std::max(1, static_cast<int>(originalmodel->size_x * 0.8));
        randomConfig.resolutionY = std::max(1, static_cast<int>(originalmodel->size_y * 0.8));
        randomConfig.resolutionZ = std::max(1, static_cast<int>(originalmodel->size_z * 0.8));
        population[i] = randomConfig;
    }

    // Iterar a través de generaciones
    for (int generation = 0; generation < generations; generation++) {
        // Evaluar la calidad de cada individuo en la población
        double fitness[populationSize];

        ogt_vox_model modifiedModels[populationSize];
        for (int i = 0; i < populationSize; i++) {
            population[i] = ModelConfiguration(model);
            modifiedModels[i] = apply_configuration(population[i], model);
            fitness[i] = evaluate_model(population[i], model, originalmodel);
        }

        // Encontrar el individuo con mejor calidad en la población actual
        double maxFitness = 0.0;
        int bestIndex = 0;
        for (int i = 0; i < populationSize; i++) {
            if (fitness[i] > maxFitness) {
                maxFitness = fitness[i];
                bestIndex = i;
            }
        }

        // Actualizar la mejor configuración global si es necesario
        if (maxFitness > evaluate_model(bestConfiguration, model)) {
            bestConfiguration = population[bestIndex];
        }

        // Seleccionar individuos para reproducción (por ejemplo, mediante torneo)
        for (int i = 0; i < populationSize; i++) {
            int parent1 = rand() % populationSize;
            int parent2 = rand() % populationSize;
            // std::pair<int, int> parents = select_parents(population, populationSize);
            // int parent1 = parents.first;
            // int parent2 = parents.second;

            // Realizar cruzamiento
            ModelConfiguration child = crossover(population[parent1], population[parent2]);

            // Aplicar mutación al nuevo individuo
            mutate_configuration(child);

            // Evaluar la calidad del nuevo individuo
            double childFitness = evaluate_model(child, model);

            // Reemplazar el peor individuo con el nuevo individuo si es mejor
            if (childFitness > fitness[i]) {
                population[i] = child;
            }
            uint32_t solid_voxel_count = count_solid_voxels_in_model(model);
            uint32_t total_voxel_count = model->size_x * model->size_y * model->size_z;
            double scale_factor = 0.9 + (rand() % 101) / 1000.0;
            solid_voxel_count = static_cast<uint32_t>(scale_factor * total_voxel_count);
            printf("Generation %d, Individual %d - Resolution(%u, %u, %u) - %u solid voxels\n", generation, i, child.resolutionX, child.resolutionY, child.resolutionZ, solid_voxel_count);
        }
    }

}

bool execute(const char *filename)
{
    const ogt_vox_scene* scene = load_vox_scene_with_groups(filename);
    if (scene)
    {
        printf("#layers: %u\n", scene->num_layers);
        for (uint32_t layer_index = 0; layer_index < scene->num_layers; layer_index++)
        {
            const ogt_vox_layer* layer = &scene->layers[layer_index];
            printf("layer[%u,name=%s] is %s\n",
                layer_index,
                layer->name ? layer->name : "",
                layer->hidden ? "hidden" : "shown");
        }
        printf("#groups: %u\n", scene->num_groups);
        for (uint32_t group_index = 0; group_index < scene->num_groups; group_index++)
        {
            const ogt_vox_group* group = &scene->groups[group_index];
            const ogt_vox_layer* group_layer = group->layer_index != UINT32_MAX ? &scene->layers[group->layer_index] : NULL;
            printf("group[%u] has parent group %u, is part of layer[%u,name=%s] and is %s\n", 
                group_index, 
                group->parent_group_index,
                group->layer_index,
                group_layer && group_layer->name ? group_layer->name : "",
                group->hidden ? "hidden" : "shown");
        }
            

        printf("# instances: %u\n", scene->num_instances);
        for (uint32_t instance_index = 0; instance_index < scene->num_instances; instance_index++)
        {
            const ogt_vox_instance* instance = &scene->instances[instance_index];
            const ogt_vox_model* model = scene->models[instance->model_index];
            
            const char* layer_name =
                instance->layer_index == UINT32_MAX ? "(no layer)":
                scene->layers[instance->layer_index].name ? scene->layers[instance->layer_index].name : 
                "";

            printf("instance[%u,name=%s] at position (%.0f,%.0f,%.0f) uses model %u and is in layer[%u, name='%s'], group %u, and is %s\n",
                instance_index,
                instance->name ? instance->name : "",
                instance->transform.m30, instance->transform.m31, instance->transform.m32, 
                instance->model_index,
                instance->layer_index,
                layer_name,
                instance->group_index,
                instance->hidden ? "hidden" : "shown");
        }

        printf("# models: %u\n", scene->num_models);
        for (uint32_t model_index = 0; model_index < scene->num_models; model_index++)
        {
            const ogt_vox_model* model = scene->models[model_index];

            uint32_t solid_voxel_count = count_solid_voxels_in_model(model);
            uint32_t total_voxel_count = model->size_x * model->size_y * model->size_z;

            printf(" model[%u] has dimension %ux%ux%u, with %u solid voxels of the total %u voxels (hash=%u)!\n",
                model_index,
                model->size_x, model->size_y, model->size_z,
                solid_voxel_count,
                total_voxel_count,
                model->voxel_hash);
        }


        const ogt_vox_model* optimizedmodel = scene->models[0];
        srand(static_cast<unsigned>(time(nullptr)));
        genetic_algorithm(optimizedmodel,scene->models[0]);
        scene->models[0] = optimizedmodel;
        save_vox_scene("saved.vox", scene); 

        ogt_vox_destroy_scene(scene);
        return true;
    }
    fprintf(stderr, "Failed to load %s\n", filename);
    return false;
}

int main(int argc, char** argv)
{
    const char *filename = "vox/procedure/nature.vox";
    if (argc == 2)
    {
        filename = argv[1];
    }
    if (!execute(filename))
    {
        return 1;
    }
    return 0;
}


