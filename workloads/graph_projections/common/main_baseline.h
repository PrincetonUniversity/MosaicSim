int main(int argc, char** argv) {

  char *x_to_y_fname;
  bgraph x_to_y_bgraph;
  weight_type *y_project;
  
  assert(argc == 2);
  x_to_y_fname = argv[1];
  x_to_y_bgraph = parse_bgraph(x_to_y_fname);
  y_project = (weight_type*) calloc(get_projection_size(x_to_y_bgraph), sizeof(weight_type));
  assert(y_project); 
  
  preprocess(x_to_y_bgraph, y_project);
  
  auto start = std::chrono::system_clock::now();

  printf("Running kernel\n");  
  _kernel_(x_to_y_bgraph, y_project);

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

  double total = 0.0;
  for (int i = 0; i < get_projection_size(x_to_y_bgraph); i++) {
    total += y_project[i];
  }
  printf("Finished hash: %.3f\n", total);

  clean_bgraph(x_to_y_bgraph);
  free(y_project);

  return 1;
}
