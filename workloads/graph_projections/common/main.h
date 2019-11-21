

int main(int argc, char** argv) {

  char *x_to_y_fname, *y_to_x_fname;
  bgraph x_to_y_bgraph, y_to_x_bgraph;

  weight_type *y_project, *x_project;
  
  assert(argc == 2);

  x_to_y_fname = argv[1];
  //y_to_x_fname = argv[2];

  //printf("first arg %s\n", x_to_y_fname);
  //printf("second arg %s\n",y_to_x_fname);

  x_to_y_bgraph = parse_bgraph(x_to_y_fname);
  //y_to_x_bgraph = parse_bgraph(y_to_x_fname);


  
  y_project = (weight_type*) calloc(get_projection_size(x_to_y_bgraph), sizeof(weight_type));
  assert(y_project);
  //x_project = (weight_type*) calloc(get_projection_size(y_to_x_bgraph), sizeof(weight_type));
  //assert(x_project);
  //cout << get_projection_size(y_to_x_bgraph) << "\n";

  preprocess(x_to_y_bgraph, y_project);
  //preprocess(y_to_x_bgraph, x_project);

  auto start = std::chrono::system_clock::now();
  
  graph_project(x_to_y_bgraph, y_project);
  auto end1 = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end1-start;

  std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

  double total = 0.0;
  for (int i = 0; i < get_projection_size(x_to_y_bgraph); i++) {
    total += y_project[i];
  }
  printf("hash first: %.3f\n", total);

  /*total = 0.0;
  for (int i = 0; i < get_projection_size(y_to_x_bgraph); i++) {
    total += x_project[i];
  }
  printf("finished hash first: %.3f\n", total);*/


  

  clean_bgraph(x_to_y_bgraph);
  //clean_bgraph(y_to_x_bgraph);
  free(y_project);
  //free(x_project);
 
  return 1;
}
