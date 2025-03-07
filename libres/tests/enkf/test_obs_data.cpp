#include <string>

#include "catch2/catch.hpp"
#include <Eigen/Dense>

#include <ert/analysis/ies/ies.hpp>
#include <ert/enkf/enkf_util.hpp>
#include <ert/enkf/obs_data.hpp>

// std::normal_distribution has a different implementation depending on platform,
// which makes this differentiation necessary.
// See following issue for details:
// https://github.com/equinor/ert/issues/2351#issuecomment-972880776
#if defined(__linux__)
std::string PLATFORM_NAME = "linux";
#elif defined(__APPLE__)
std::string PLATFORM_NAME = "macos";
#endif

/*
The following Python code reproduces numbers used in this test:

E = np.array([[1.189337, 0.888379, 0.615577],
             [-0.579617, -0.404989, -0.317147],
             [-0.640415, 1.044720, 0.217470],
             [-0.049693, -0.641603, 0.009214],
             [0.777625, 0.084298, 0.295595]])

nens = E.shape[1]
nobs = E.shape[0]

E_centered = E - E.mean(axis=1).reshape(nobs, 1)

pert_var = (E_centered * E_centered).sum(axis=1)

obs_block_std = np.array([0.3, 0.5, 0.3, 0.5, 0.6])

factor = obs_block_std.reshape(nobs, 1) * np.sqrt(
    nens / pert_var.reshape(nobs, 1)
)

E_final = E_centered * factor

In general, E is initially drawn from a normal distribution:

nens = 3
nobs = 5
E = rng.normal(0, 1, size=(nobs, nens))

*/
SCENARIO(
    "E-matrix is initialized using a normal distribution and scaled with data",
    "[obs_data_makeE]") {
    GIVEN("A obs_data with one obs_block and an instance of rng") {
        double global_std_scaling = 1.0;
        obs_data_type *obs_data = obs_data_alloc(global_std_scaling);

        const char *obs_key = "obs_block_0";
        const int obs_size = 3;
        obs_block_type *obs_block =
            obs_data_add_block(obs_data, obs_key, obs_size);
        obs_block_iset(obs_block, 0, 5.0, 0.3);
        obs_block_iset(obs_block, 2, 15.0, 0.5);

        REQUIRE(obs_data_get_active_size(obs_data) == 2);
        REQUIRE(obs_data_get_num_blocks(obs_data) == 1);

        auto rng = rng_alloc(MZRAN, INIT_DEFAULT);

        WHEN("E is allocated") {

            int active_obs_size = obs_data_get_active_size(obs_data);
            int active_ens_size = 3;
            Eigen::MatrixXd noise =
                Eigen::MatrixXd::Zero(active_obs_size, active_ens_size);
            for (int j = 0; j < active_ens_size; j++)
                for (int i = 0; i < active_obs_size; i++)
                    noise(i, j) = enkf_util_rand_normal(0, 1, rng);
            Eigen::VectorXd observation_errors =
                obs_data_errors_as_vector(obs_data) * global_std_scaling;
            Eigen::MatrixXd E = ies::makeE(observation_errors, noise);

            if (PLATFORM_NAME == "macos") {
                THEN("Rows of E are effected by data in the block") {
                    REQUIRE(E(0, 0) == Approx(0.285993));
                    REQUIRE(E(0, 1) == Approx(-0.414393));
                    REQUIRE(E(0, 2) == Approx(0.128400));
                    REQUIRE(E(1, 0) == Approx(-0.548597));
                    REQUIRE(E(1, 1) == Approx(-0.112071));
                    REQUIRE(E(1, 2) == Approx(0.660668));
                }
            } else if (PLATFORM_NAME == "linux") {
                THEN("Rows of E are effected by data in the block") {
                    REQUIRE(E(0, 0) == Approx(-0.143983));
                    REQUIRE(E(0, 1) == Approx(0.417609));
                    REQUIRE(E(0, 2) == Approx(-0.273627));
                    REQUIRE(E(1, 0) == Approx(-0.248757));
                    REQUIRE(E(1, 1) == Approx(0.697606));
                    REQUIRE(E(1, 2) == Approx(-0.448849));
                }
            }
        }

        WHEN("One more block is added and E is allocated") {
            const char *obs_key = "obs_block_1";
            const int obs_size = 4;
            obs_block_type *obs_block2 =
                obs_data_add_block(obs_data, obs_key, obs_size);
            obs_block_iset(obs_block2, 0, 5.0, 0.3);
            obs_block_iset(obs_block2, 1, 15.0, 0.5);
            obs_block_iset(obs_block2, 2, 20.0, 0.6);

            REQUIRE(obs_data_get_active_size(obs_data) == 5);
            REQUIRE(obs_data_get_num_blocks(obs_data) == 2);

            int active_obs_size = obs_data_get_active_size(obs_data);
            int active_ens_size = 3;
            Eigen::MatrixXd noise =
                Eigen::MatrixXd::Zero(active_obs_size, active_ens_size);
            for (int j = 0; j < active_ens_size; j++)
                for (int i = 0; i < active_obs_size; i++)
                    noise(i, j) = enkf_util_rand_normal(0, 1, rng);
            Eigen::VectorXd observation_errors =
                obs_data_errors_as_vector(obs_data) * global_std_scaling;
            Eigen::MatrixXd E = ies::makeE(observation_errors, noise);

            if (PLATFORM_NAME == "macos") {
                THEN("Rows of E are effected by data in the block") {
                    REQUIRE(E(0, 0) == Approx(0.373284));
                    REQUIRE(E(0, 1) == Approx(-0.012016));
                    REQUIRE(E(0, 2) == Approx(-0.361268));
                    REQUIRE(E(1, 0) == Approx(-0.667806));
                    REQUIRE(E(1, 1) == Approx(0.132592));
                    REQUIRE(E(1, 2) == Approx(0.535214));
                    REQUIRE(E(2, 0) == Approx(-0.369630));
                    REQUIRE(E(2, 1) == Approx(0.365177));
                    REQUIRE(E(2, 2) == Approx(0.004453).epsilon(0.0001));
                    REQUIRE(E(3, 0) == Approx(0.302260));
                    REQUIRE(E(3, 1) == Approx(-0.704736));
                    REQUIRE(E(3, 2) == Approx(0.402476));
                    REQUIRE(E(4, 0) == Approx(0.810162));
                    REQUIRE(E(4, 1) == Approx(-0.623549));
                    REQUIRE(E(4, 2) == Approx(-0.186613));
                }
            } else if (PLATFORM_NAME == "linux") {
                THEN("Rows of E are effected by data in the block") {
                    REQUIRE(E(0, 0) == Approx(-0.075665));
                    REQUIRE(E(0, 1) == Approx(-0.323700));
                    REQUIRE(E(0, 2) == Approx(0.399366));
                    REQUIRE(E(1, 0) == Approx(-0.166335));
                    REQUIRE(E(1, 1) == Approx(0.678356));
                    REQUIRE(E(1, 2) == Approx(-0.512021));
                    REQUIRE(E(2, 0) == Approx(0.130487));
                    REQUIRE(E(2, 1) == Approx(0.284370));
                    REQUIRE(E(2, 2) == Approx(-0.414857));
                    REQUIRE(E(3, 0) == Approx(-0.497966));
                    REQUIRE(E(3, 1) == Approx(0.683750));
                    REQUIRE(E(3, 2) == Approx(-0.185784));
                    REQUIRE(E(4, 0) == Approx(-0.826843));
                    REQUIRE(E(4, 1) == Approx(0.578492));
                    REQUIRE(E(4, 2) == Approx(0.248351));
                }
            }
        }
        rng_free(rng);
        obs_data_free(obs_data);
    }
}

SCENARIO("Creating eigen vectors from obs_data [obs_data]") {
    GIVEN("A obs_data with one obs_block") {
        double global_std_scaling = 1.0;
        obs_data_type *obs_data = obs_data_alloc(global_std_scaling);

        const char *obs_key = "obs_block_0";
        const int obs_size = 3;

        obs_block_type *obs_block =
            obs_data_add_block(obs_data, obs_key, obs_size);
        obs_block_iset(obs_block, 0, 5.0, 0.3);
        obs_block_iset(obs_block, 2, 15.0, 0.5);

        THEN("loading as vector") {
            Eigen::VectorXd observation_errors =
                obs_data_errors_as_vector(obs_data);
            Eigen::VectorXd observation_values =
                obs_data_values_as_vector(obs_data);
            REQUIRE(observation_values == Eigen::Vector2d(5.0, 15.0));
            REQUIRE(observation_errors == Eigen::Vector2d(0.3, 0.5));
        }
    }
}
