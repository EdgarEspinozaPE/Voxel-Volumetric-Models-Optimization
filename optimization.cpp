#define VOX_IMPLEMENTATION
#include "vox.h"

#if defined(_MSC_VER)
    #include <io.h>
#endif
#include <stdio.h>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <vector>

const vox_scene* load_vox_scene(const char* filename, uint32_t scene_read_flags = 0)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE * fp;
    if (0 != fopen_s(&fp, filename, "rb"))
        fp = 0;
#else
    FILE * fp = fopen(filename, "rb");
#endif
    if (!fp)
        return NULL;

    fseek(fp, 0, SEEK_END);
    uint32_t buffer_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint8_t * buffer = new uint8_t[buffer_size];
    fread(buffer, buffer_size, 1, fp);
    fclose(fp);

    const vox_scene * scene = vox_read_scene_with_flags(buffer, buffer_size, scene_read_flags);

    delete[] buffer;

    return scene;
}

const vox_scene* load_vox_scene_with_groups(const char* filename)
{
    return load_vox_scene(filename, k_read_scene_flags_groups);
}


void save_vox_scene(const char* pcFilename, const vox_scene* scene) 
{ 
    uint32_t buffersize = 0;
    uint8_t* buffer = vox_write_scene(scene, &buffersize);
    if (!buffer)
        return;

#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE * fp;
    if (0 != fopen_s(&fp, pcFilename, "wb"))
        fp = 0;
#else
    FILE * fp = fopen(pcFilename, "wb");
#endif
    if (!fp) {
        vox_free(buffer);
        return;
    }

    fwrite(buffer, buffersize, 1, fp);
    fclose(fp);
    vox_free(buffer);
}

uint32_t count_solid_voxels_in_model(const vox_model* model)
{
    uint32_t solid_voxel_count = 0;
    uint32_t voxel_index = 0;
    for (uint32_t z = 0; z < model->size_z; z++) {
        for (uint32_t y = 0; y < model->size_y; y++) {
            for (uint32_t x = 0; x < model->size_x; x++, voxel_index++) {
                uint32_t color_index = model->voxel_data[voxel_index];
                bool is_voxel_solid = (color_index != 0);
                solid_voxel_count += (is_voxel_solid ? 1 : 0);
            }
        }
    }
    return solid_voxel_count;
}

struct ModelConfiguration {
    uint32_t resolutionX;
    uint32_t resolutionY;
    uint32_t resolutionZ;
    const uint8_t* voxel_data;
    int material;

    ModelConfiguration() {
        resolutionX = rand() % 25 + 1;
        resolutionY = rand() % 25 + 1;
        resolutionZ = rand() % 25 + 1;
        material = rand() % 6;
    }
    ModelConfiguration(const vox_model* model) {
        resolutionX = model->size_x;
        resolutionY = model->size_y;
        resolutionZ = model->size_z;
        voxel_data = model->voxel_data;
        material = 2; 
    }
};

vox_model apply_configuration(const ModelConfiguration& configuration, const vox_model* originalModel) {
    vox_model modifiedModel = *originalModel;

    modifiedModel.size_x = configuration.resolutionX;
    modifiedModel.size_y = configuration.resolutionY;
    modifiedModel.size_z = configuration.resolutionZ;

    return modifiedModel;
}

double evaluate_model(const ModelConfiguration& configuration, const vox_model* model) {
    double resolutionQuality = static_cast<double>(configuration.resolutionX * configuration.resolutionY * configuration.resolutionZ);

    return resolutionQuality;
}

double evaluate_model(const ModelConfiguration& configuration, const vox_model* originalModel, const vox_model* modifiedModel) {

    double colorDifference = 0.0;
    for (uint32_t i = 0; i < originalModel->size_x * originalModel->size_y * originalModel->size_z; i++) {
        uint8_t originalColor = originalModel->voxel_data[i];
        uint8_t modifiedColor = modifiedModel->voxel_data[i];
        colorDifference += std::abs(originalColor - modifiedColor);
    }


    double originalWeight = static_cast<double>(originalModel->size_x * originalModel->size_y * originalModel->size_z);
    double modifiedWeight = static_cast<double>(configuration.resolutionX * configuration.resolutionY * configuration.resolutionZ);
    double weightReduction = originalWeight - modifiedWeight;


    double visualQuality = 1.0 - (colorDifference / (255.0 * originalModel->size_x * originalModel->size_y * originalModel->size_z));

    double totalFitness = 0.7 * visualQuality + 0.3 * weightReduction;

    return totalFitness;
}


void mutate_configuration(ModelConfiguration& configuration) {

    int randomValue = rand() % 2;

    if (randomValue == 0) {
        configuration.material = 0; 
    }
    // Otros posibles ajustes/mutaciones
    // ...
}

ModelConfiguration crossover(const ModelConfiguration& parent1, const ModelConfiguration& parent2) {
    ModelConfiguration child;

    int crossoverPointX = rand() % 2;
    int crossoverPointY = rand() % 2;
    int crossoverPointZ = rand() % 2;

    child.resolutionX = (crossoverPointX == 0) ? parent1.resolutionX : parent2.resolutionX;
    child.resolutionY = (crossoverPointY == 0) ? parent1.resolutionY : parent2.resolutionY;
    child.resolutionZ = (crossoverPointZ == 0) ? parent1.resolutionZ : parent2.resolutionZ;

    int reductionFactor = 5;
    int totalVoxels = child.resolutionX * child.resolutionY * child.resolutionZ;
    int voxelsToRemove = totalVoxels / reductionFactor;

    const uint8_t* voxelDataParent1 = parent1.voxel_data;
    const uint8_t* voxelDataParent2 = parent2.voxel_data;
    uint8_t* childVoxelData = new uint8_t[child.resolutionX * child.resolutionY * child.resolutionZ];

    for (int i = 0; i < child.resolutionX * child.resolutionY * child.resolutionZ; ++i) {
        if ((i % 2) == 0) {
            childVoxelData[i] = voxelDataParent1[i];
        } else {
            childVoxelData[i] = voxelDataParent2[i];
        }
    }

    return child;
}

double resolution_difference(const ModelConfiguration& config1, const ModelConfiguration& config2) {
    double diffX = std::abs(static_cast<double>(config1.resolutionX - config2.resolutionX));
    double diffY = std::abs(static_cast<double>(config1.resolutionY - config2.resolutionY));
    double diffZ = std::abs(static_cast<double>(config1.resolutionZ - config2.resolutionZ));
    return diffX + diffY + diffZ;
}


std::pair<int, int> select_parents(const ModelConfiguration* population, int populationSize) {

    std::vector<double> similarityScores(populationSize * (populationSize - 1) / 2);
    int scoreIndex = 0;
    for (int i = 0; i < populationSize - 1; ++i) {
        for (int j = i + 1; j < populationSize; ++j) {
            similarityScores[scoreIndex++] = resolution_difference(population[i], population[j]);
        }
    }

    double totalScore = std::accumulate(similarityScores.begin(), similarityScores.end(), 0.0);
    std::vector<double> selectionProbabilities(populationSize);
    for (int i = 0; i < populationSize; ++i) {
        double modelScore = 0.0;
        for (int j = 0; j < populationSize; ++j) {
            if (i != j) {
                modelScore += totalScore / similarityScores[j * (populationSize - 1) + (i < j ? i : i - 1)];
            }
        }
        selectionProbabilities[i] = modelScore;
    }

    int parent1 = 0, parent2 = 0;
    double maxProb1 = 0.0, maxProb2 = 0.0;
    for (int i = 0; i < populationSize; ++i) {
        double randomProb = static_cast<double>(rand()) / RAND_MAX;
        if (randomProb > maxProb1) {
            maxProb1 = randomProb;
            parent1 = i;
        }
        if (randomProb > maxProb2 && i != parent1) {
            maxProb2 = randomProb;
            parent2 = i;
        }
    }

    return std::make_pair(parent1, parent2);
}


void genetic_algorithm(const vox_model* model, const vox_model* originalmodel) {
    const int populationSize = 50;
    const int generations = 100;

    ModelConfiguration population[populationSize];
    ModelConfiguration bestConfiguration;

    population[0] = ModelConfiguration(originalmodel);

    for (int i = 1; i < populationSize; i++) {

        ModelConfiguration randomConfig;
        randomConfig.resolutionX = std::max(1, static_cast<int>(originalmodel->size_x * 0.8));
        randomConfig.resolutionY = std::max(1, static_cast<int>(originalmodel->size_y * 0.8));
        randomConfig.resolutionZ = std::max(1, static_cast<int>(originalmodel->size_z * 0.8));
        population[i] = randomConfig;
    }


    for (int generation = 0; generation < generations; generation++) {
        double fitness[populationSize];

        vox_model modifiedModels[populationSize];
        for (int i = 0; i < populationSize; i++) {
            population[i] = ModelConfiguration(model);
            modifiedModels[i] = apply_configuration(population[i], model);
            fitness[i] = evaluate_model(population[i], model, originalmodel);
        }

        double maxFitness = 0.0;
        int bestIndex = 0;
        for (int i = 0; i < populationSize; i++) {
            if (fitness[i] > maxFitness) {
                maxFitness = fitness[i];
                bestIndex = i;
            }
        }

        if (maxFitness > evaluate_model(bestConfiguration, model)) {
            bestConfiguration = population[bestIndex];
        }

        for (int i = 0; i < populationSize; i++) {
            std::pair<int, int> parents = select_parents(population, populationSize);
            int parent1 = parents.first;
            int parent2 = parents.second;

            ModelConfiguration child = crossover(population[parent1], population[parent2]);

            mutate_configuration(child);

            double childFitness = evaluate_model(child, model);

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
    const vox_scene* scene = load_vox_scene_with_groups(filename);
    if (scene)
    {
 
        printf("# models: %u\n", scene->num_models);
        for (uint32_t model_index = 0; model_index < scene->num_models; model_index++)
        {
            const vox_model* model = scene->models[model_index];

            uint32_t solid_voxel_count = count_solid_voxels_in_model(model);
            uint32_t total_voxel_count = model->size_x * model->size_y * model->size_z;

            printf(" model[%u] has dimension %ux%ux%u, with %u solid voxels of the total %u voxels (hash=%u)!\n",
                model_index,
                model->size_x, model->size_y, model->size_z,
                solid_voxel_count,
                total_voxel_count,
                model->voxel_hash);
        }


        const vox_model* optimizedmodel = scene->models[0];
        genetic_algorithm(optimizedmodel,scene->models[0]);
        scene->models[0] = optimizedmodel;
        save_vox_scene("saved.vox", scene); 

        vox_destroy_scene(scene);
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


