/*
 * Copyright (C) 2024 Sebastian Pilarz
 *
 * This program is licensed under the GNU Affero General Public License.
 * You should have received a copy of this license along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include <random>
#include <vector>
class GameOfLife {
public:
  GameOfLife(int x, int y, std::uint_fast32_t seed = 0) {
    grid = std::vector<std::vector<bool>>(x, std::vector<bool>(y, false));
    this->seed > 0 ? seed : std::random_device{}();
    std::mt19937 gen(this->seed);
    // std::uniform_int_distribution<> distrib(0, 1);
    // std::bernoulli_distribution distrib(0.05);
    std::bernoulli_distribution distrib(0.2);

    for (int i = 0; i < x; ++i) {
      for (int j = 0; j < y; ++j) {
        grid[i][j] = distrib(gen) > 0;
      }
    }
  }

  void iterate() {
    std::vector<std::vector<bool>> new_grid = grid;
    for (int i = 0; i < grid.size(); i++) {
      for (int j = 0; j < grid[0].size(); j++) {
        int count = moore_neighborhood(i, j);
        if (grid[i][j]) {
          if (count < 2 || count > 3) {
            new_grid[i][j] = false;
          }
        } else {
          if (count == 3) {
            new_grid[i][j] = true;
          }
        }
      }
    }
    grid = new_grid;
    iterations++;
  }

  bool get_cell(int x, int y) { return grid[x][y]; }
  void set_cell(int x, int y, bool value) { grid[x][y] = value; }
  int get_cols_num() { return grid.size(); }
  int get_rows_num() { return grid[0].size(); }
  std::uint_fast32_t get_seed() { return seed; }
  size_t get_iterations() { return iterations; }

private:
  std::vector<std::vector<bool>> grid;
  std::uint_fast32_t seed = 0;
  size_t iterations = 0;

  int moore_neighborhood(int x, int y) {
    int w = get_cols_num();
    int h = get_rows_num();
    int count = 0;
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        if (i == 0 && j == 0) {
          continue;
        }
        // if (x + i >= 0 && x + i < grid.size() && y + j >= 0 &&
        //     y + j < grid[0].size()) {
        //   count += grid[x + i][y + j]; // true = 1, false = 0
        // }
        int nx = (x + i + w) % w;
        int ny = (y + j + h) % h;
        count += grid[nx][ny]; // true = 1, false = 0
      }
    }
    return count;
  }
};